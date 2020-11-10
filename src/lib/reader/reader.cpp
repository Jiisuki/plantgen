/** @file
 *  @brief Implementation of the reader class.
 */

#include "reader.h"

#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>

Reader_t::Reader_t(Collection* collection, std::string filename, const bool verbose)
{
    this->verbose = verbose;
    this->collection = collection;
    this->in.open(filename);
    if ((true != this->in.is_open()) || (nullptr == collection))
    {
        // error
    }
    else
    {
        // set default model name
        size_t dot_index = filename.find_last_of(".");
        collection->set_model_name(filename.substr(0, dot_index));
    }
}

Reader_t::~Reader_t()
{
    if (this->in.is_open())
    {
        this->in.close();
    }
}

void Reader_t::compile_collection()
{
    if ((nullptr != collection) && (in.is_open()))
    {
        if (verbose)
        {
            std::cout << "Compiling collection..." << std::endl;
        }

        collect_states();
    }
    else
    {
        if (verbose)
        {
            std::cout << "Can not compile collection..." << std::endl;
        }
    }
}

bool Reader_t::is_tr_arrow(const std::string& token)
{
    bool is_arrow = false;
    char first = token.front();
    char last = token.back();
    if (('-' == first) && ('>' == last))
    {
        is_arrow = true;
    }
    return (is_arrow);
}

// private function to collect all states in the document
void Reader_t::collect_states()
{
    if (verbose)
    {
        std::cout << "Collecting states..." << std::endl;
    }

    // rewind
    in.clear();
    in.seekg(0);

    std::vector<unsigned int> parent_nesting;
    unsigned int parent_state = 0;

    std::string str;
    bool is_uml = false;
    bool is_header = false;
    bool is_footer = false;

    while (std::getline(this->in, str))
    {
        /* Need to eliminate the new-line. */
        str = str.substr(0, str.length()-1);

        if (!is_uml && ("@startuml" == str))
        {
            // start parsing
            if (verbose)
            {
                std::cout << "Found UML start tag." << std::endl;
            }
            is_uml = true;
        }
        else if (is_uml && ("@enduml" == str))
        {
            // end parsing
            if (verbose)
            {
                std::cout << "Found UML end tag." << std::endl;
            }
            is_uml = false;
        }
        else if (is_uml)
        {
            add_uml_line(str);

            if ("header" == str)
            {
                // TODO: start header parsing
                is_header = true;
            }
            else if ("footer" == str)
            {
                // TODO: start footer parsing
                is_footer = true;
            }
            else if ("endheader" == str)
            {
                // TODO: end header parsing
                is_header = false;
            }
            else if ("endfooter" == str)
            {
                // TODO: end footer parsing
                is_footer = false;
            }
            else if (is_header || is_footer)
            {
                // parse header/footer
                auto tokens = tokenize(str);

                if (!tokens.empty())
                {
                    if (("model" == tokens[0]) && (2 == tokens.size()))
                    {
                        collection->set_model_name(tokens[1]);

                        if (verbose)
                        {
                            std::cout << "Model name detected: " << collection->get_model_name() << std::endl;
                        }
                    }
                    else if (("import" == tokens[0]) && (3 <= tokens.size()))
                    {
                        Import i {};
                        if (("global" == tokens[1]) && (4 == tokens.size()))
                        {
                            i.is_global = true;
                            i.content = tokens[3];
                        }
                        else
                        {
                            i.is_global = false;
                            i.content = tokens[2];
                        }
                        add_import(i);
                    }
                    else if ((("private" == tokens[0]) || ("public" == tokens[0])) && (5 <= tokens.size()))
                    {
                        Variable v;
                        v.is_private = ("private" == tokens[0]) ? true : false;
                        v.name = tokens[2];
                        v.type = tokens[4];

                        if (7 == tokens.size())
                        {
                            v.has_specific_initial_value = true;
                            v.initial_value = tokens[6];
                        }
                        else
                        {
                            v.has_specific_initial_value = false;
                            v.initial_value = "";
                        }
                        add_variable(v);
                    }
                    else if ((("in" == tokens[0]) || ("out" == tokens[0])) && ("event" == tokens[1]) && (3 <= tokens.size()))
                    {
                        Event e;
                        e.name = tokens[2];
                        // time config
                        e.is_time_event = false;
                        e.expire_time_ms = 0;
                        e.is_periodic = false;

                        if (5 == tokens.size())
                        {
                            e.require_parameter = true;
                            e.parameter_type = tokens[4];
                        }
                        else
                        {
                            e.require_parameter = false;
                            e.parameter_type = "";
                        }

                        if ("in" == tokens[0])
                        {
                            e.direction = EventDirection::Incoming;
                        }
                        else
                        {
                            e.direction = EventDirection::Outgoing;
                        }

                        add_event(e);
                    }
                    else if (("event" == tokens[0]) && (2 <= tokens.size()))
                    {
                        Event e;
                        e.name = tokens[1];
                        // time config
                        e.is_time_event = false;
                        e.expire_time_ms = 0;
                        e.is_periodic = false;

                        if (4 == tokens.size())
                        {
                            e.require_parameter = true;
                            e.parameter_type = tokens[3];
                        }
                        else
                        {
                            e.require_parameter = false;
                            e.parameter_type = "";
                        }

                        e.direction = EventDirection::Internal;
                        add_event(e);
                    }
                }
            }
            else
            {
                // parse line
                auto tokens = tokenize(str);
                if (!tokens.empty())
                {
                    // =========================================================
                    // STATE DEFINITION # state X Y
                    // =========================================================
                    if (("state" == tokens[0]) && (1 < tokens.size()))
                    {
                        // define a state
                        State s;
                        s.name = tokens[1];
                        s.parent = parent_state;
                        s.is_choice = false;

                        bool set_new_parent = false;
                        if (2 < tokens.size())
                        {
                            // check for special
                            if ("<<choice>>" == tokens[2])
                            {
                                s.is_choice = true;
                            }
                            else if ("{" == tokens[2])
                            {
                                // parent nesting
                                set_new_parent = true;
                            }
                        }

                        const unsigned int id = add_state(s);
                        if (set_new_parent)
                        {
                            if (0 != parent_state)
                            {
                                parent_nesting.push_back(parent_state);
                            }
                            parent_state = id;
                        }
                    }

                    // =========================================================
                    // STATE TRANSITION # S1 -> S2 : X Y
                    // If X is a guard [X] then [Y] is considered a part of this
                    // If X is an event X then [Y] is considered its guard
                    // =========================================================
                    else if ((2 < tokens.size()) && (is_tr_arrow(tokens[1])))
                    {
                        State A {}, B {};
                        A.name = (0 == tokens[0].compare("[*]")) ? "initial" : tokens[0];
                        A.is_choice = false; // relies on already defined state.
                        A.parent = parent_state;

                        B.name = (0 == tokens[2].compare("[*]")) ? "final" : tokens[2];
                        B.is_choice = false; // see above.
                        B.parent = parent_state;

                        const unsigned int idA = add_state(A);
                        const unsigned int idB = add_state(B);

                        Event e;
                        e.name = "null";
                        e.direction = EventDirection::Incoming; // assume default
                        e.parameter_type = "";
                        e.require_parameter = false;
                        e.is_time_event = false;
                        e.expire_time_ms = 0;
                        e.is_periodic = false;

                        Transition t;
                        t.state_a = idA;
                        t.state_b = idB;
                        t.has_guard = false;
                        t.guard_content = "";

                        if ((4 < tokens.size()) && (":" == tokens[3]))
                        {
                            if ('[' == tokens[4].front())
                            {
                                // guard only transition.
                                t.has_guard = true;
                                std::string guardStr = tokens[4];
                                for (size_t i = 5; i < tokens.size(); i++)
                                {
                                    guardStr += " " + tokens[i];
                                }
                                t.guard_content = guardStr.substr(1, guardStr.length()-2);
                            }
                            else
                            {
                                // check for timed event
                                if (("after" == tokens[4]) || ("every" == tokens[4]))
                                {
                                    // S1 -> S2 : after X u [Y]
                                    // 0  1  2  3 4     5 6 7 - index
                                    // 1  2  3  4 5     6 7 8 - count
                                    e.is_time_event = true;
                                    e.name = A.name + "_" + tokens[4] + "_";
                                    // append time unit to time event name
                                    for (size_t i = 5; i < std::min(tokens.size(), (size_t) 7); i++)
                                    {
                                        e.name += tokens[i];
                                    }

                                    if (6 < tokens.size())
                                    {
                                        size_t multiplier = 1;
                                        if ("s" == tokens[6])
                                        {
                                            multiplier = 1000;
                                        }
                                        else if ("min" == tokens[6])
                                        {
                                            multiplier = 60000;
                                        }

                                        e.expire_time_ms = multiplier * ((unsigned int) std::stoul(tokens[5]));

                                        if ((7 < tokens.size()) && ('[' == tokens[7].front()))
                                        {
                                            // guard on transition
                                            t.has_guard = true;
                                            std::string guardStr = tokens[7];
                                            // get remainding guard string
                                            for (size_t i = 8; i < tokens.size(); i++)
                                            {
                                                guardStr += " " + tokens[i];
                                            }
                                            t.guard_content = guardStr.substr(1, guardStr.length()-2);
                                        }
                                    }
                                    else
                                    {
                                        std::cout << "ERROR: No time specified on time event." << std::endl;
                                        // TODO: Error.
                                    }
                                }
                                else
                                {
                                    // normal event
                                    e.name = tokens[4];
                                    if ((5 < tokens.size()) && ('[' == tokens[5].front()))
                                    {
                                        // guard on transition.
                                        t.has_guard = true;
                                        std::string guardStr = tokens[5];
                                        for (size_t i = 6; i < tokens.size(); i++)
                                        {
                                            guardStr += " " + tokens[i];
                                        }
                                        t.guard_content = guardStr.substr(1, guardStr.length()-2);
                                    }
                                }
                            }
                        }

                        // add the event
                        t.event = add_event(e);

                        // add event to transition and add transition.
                        add_transition(t);
                    }

                    // =========================================================
                    // STATE ACTION # S : T / X
                    // T is entry/exit/oncycle and X is the action for state S.
                    // Check if X contains a 'raise' keyword followed by Y, then
                    // Y shall be added to the outgoing event list.
                    // =========================================================
                    else if ((2 < tokens.size()) && (":" == tokens[1]))
                    {
                        // action
                        unsigned int id = 0;
                        for (auto s : collection->states)
                        {
                            if (tokens[0] == s.name)
                            {
                                id = s.id;
                                break;
                            }
                        }

                        if (0 != id)
                        {
                            if ((3 < tokens.size()) && ("/" == tokens[3]))
                            {
                                State_Decl d;
                                d.state_id = id;

                                bool has_type = true;
                                if ("entry" == tokens[2])
                                {
                                    d.type = Declaration::Entry;
                                }
                                else if ("exit" == tokens[2])
                                {
                                    d.type = Declaration::Exit;
                                }
                                else if ("oncycle" == tokens[2])
                                {
                                    d.type = Declaration::OnCycle;
                                }
                                else
                                {
                                    has_type = false;
                                }

                                if (has_type)
                                {
                                    // look for any 'raise' stuff
                                    size_t i = 4;
                                    while (i < (tokens.size() - 1))
                                    {
                                        if ("raise" == tokens[i])
                                        {
                                            Event e;
                                            e.name = tokens[i+1];
                                            e.direction = EventDirection::Internal;
                                            e.require_parameter = false;
                                            e.parameter_type = "";
                                            e.is_time_event = false;
                                            e.is_periodic = false;
                                            e.expire_time_ms = 0;
                                            add_event(e);
                                        }
                                        i++;
                                    }

                                    std::string declStr = tokens[4];
                                    for (size_t i = 5; i < tokens.size(); i++)
                                    {
                                        declStr += " " + tokens[i];
                                    }
                                    d.content = declStr;
                                    add_declaration(d);
                                }
                            }
                            else
                            {
                                // comment
                                State_Decl d;
                                d.state_id = id;
                                d.type = Declaration::Comment;

                                std::string declStr = tokens[2];
                                for (size_t i = 3; i < tokens.size(); i++)
                                {
                                    declStr += " " + tokens[i];
                                }
                                d.content = declStr;
                                add_declaration(d);
                            }
                        }
                    }

                    // =========================================================
                    // CLOSE STATE (parent)
                    // =========================================================
                    else if (0 == tokens[0].compare("}"))
                    {
                        // pop back parent nesting
                        if (!parent_nesting.empty())
                        {
                            parent_state = parent_nesting.back();
                            parent_nesting.pop_back();
                        }
                        else
                        {
                            parent_state = 0;
                        }
                    }
                }
            }
        }
    }
}

unsigned int Reader_t::add_state(State& state)
{
    static unsigned int id = 0;
    unsigned int return_id = 0;

    // look for duplicate
    bool is_found = false;
    for (auto s : collection->states)
    {
        if (("initial" == state.name) || ("final" == state.name))
        {
            // check if this exact one exists already
            if ((state.name == s.name) && (state.parent == s.parent))
            {
                is_found = true;
                return_id = s.id;
                break;
            }
        }
        else
        {
            // require unique names
            if (state.name == s.name)
            {
                is_found = true;
                return_id = s.id;
                break; // for
            }
        }
    }

    if (!is_found)
    {
        // new state
        id++;
        state.id = id;
        collection->states.push_back(state);
        return_id = state.id;

        if (verbose)
        {
            std::cout << "NEW STATE: " << state.name << ", id = " << state.id << ", parent = " << state.parent << std::endl;
        }
    }

    return (return_id);
}

Event Reader_t::add_event(Event& event)
{
    static unsigned int id = 0;

    // look for duplicate
    Event ev = event;

    bool is_found = false;
    for (auto e : collection->events)
    {
        if (event.name == e.name)
        {
            is_found = true;
            ev = e;
            break; // for
        }
    }

    if (!is_found)
    {
        // new event
        id++;
        event.id = id;
        collection->events.push_back(event);
        ev = event;

        if (verbose)
        {
            std::string type = (event.is_time_event) ?
                "time" : (EventDirection::Incoming == event.direction) ?
                    "incoming" : (EventDirection::Internal == event.direction) ?
                            "internal" : "outgoing";

            std::cout << "Added new (" << type << ") event " << event.name << std::endl;
        }
    }
    else
    {
        std::cout << "Duplicate entry found for " << event.name << std::endl;
    }

    return (ev);
}

void Reader_t::add_transition(Transition& transition)
{
    static unsigned int id = 0;
    id++;
    transition.id = id;
    collection->transitions.push_back(transition);

    if (verbose)
    {
        State A = collection->get_state_by_id(transition.state_a);
        State B = collection->get_state_by_id(transition.state_b);
        std::string stateA = (0 == A.id) ? "NULL" : A.name;
        std::string stateB = (0 == B.id) ? "NULL" : B.name;
        std::string guardDesc = (transition.has_guard) ? (" with guard [" + transition.guard_content + "]") : "";
        std::cout << "Added transition "
                  << stateA
                  << " --> "
                  << stateB
                  << " on event "
                  << transition.event.name
                  << guardDesc
                  << std::endl;
    }
}

void Reader_t::add_declaration(State_Decl& decl)
{
    static unsigned int id = 0;
    id++;
    decl.id = id;
    collection->state_declarations.push_back(decl);

    if (verbose)
    {
        State st = collection->get_state_by_id(decl.state_id);
        std::string name = (0 == st.id) ? "NULL" : st.name;
        std::string type = "NULL";
        if (Declaration::Entry == decl.type)
        {
            type = "Entry";
        }
        else if (Declaration::Exit == decl.type)
        {
            type = "Exit";
        }
        else if (Declaration::OnCycle == decl.type)
        {
            type = "OnCycle";
        }
        std::cout << "Wrote " << type << " declaration for state " << name << std::endl;
    }
}

void Reader_t::add_variable(Variable& variable)
{
    static unsigned int id = 0;
    id++;
    variable.id = id;
    collection->variables.push_back(variable);

    if (verbose)
    {
        std::cout << "Found variable " << variable.type << " " << variable.name;
        if (variable.has_specific_initial_value)
        {
            std::cout << " = " << variable.initial_value << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
    }
}

void Reader_t::add_import(Import& anImport)
{
    static unsigned int id = 0;
    id++;
    anImport.id = id;
    collection->imports.push_back(anImport);

    if (verbose)
    {
        std::cout << "Found import ";
        if (anImport.is_global)
        {
            std::cout << "<" << anImport.content << ">" << std::endl;
        }
        else
        {
            std::cout << "\"" << anImport.content << "\"" << std::endl;
        }
    }
}

void Reader_t::add_uml_line(const std::string& line)
{
    collection->uml.push_back(line);
}

std::vector<std::string> Reader_t::tokenize(const std::string& str)
{
    std::vector<std::string> tokens;
    std::string tmp;
    std::istringstream iss(str);
    while (iss >> tmp)
    {
        tokens.push_back(tmp);
    }
    return (tokens);
}


