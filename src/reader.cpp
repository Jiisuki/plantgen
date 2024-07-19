/** @file
 *  @brief Implementation of the reader class.
 */

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/reader.hpp"

Reader::Reader(const std::string& filename, const bool v) : verbose(v)
{
    in.open(filename);
    if (!in.is_open())
    {
        throw std::runtime_error("Failed to open file.");
    }
    else
    {
        // set default model name
        auto index = filename.find_last_of('.');
        model_name = filename.substr(0, index);
        collect_states();
    }
}

Reader::~Reader()
{
    in.close();
}

std::string Reader::get_model_name() const
{
    return model_name;
}

size_t Reader::get_uml_line_count() const
{
    return uml.size();
}

std::string Reader::get_uml_line(const size_t i) const
{
    return uml[i];
}

size_t Reader::get_variable_count() const
{
    return variables.size();
}

Variable* Reader::get_variable(const size_t id)
{
    if (id < variables.size())
    {
        return &variables[id];
    }
    return nullptr;
}

size_t Reader::getPrivateVariableCount() const
{
    size_t n = 0;
    std::for_each(
            variables.begin(),
            variables.end(),
            [&n](const Variable& v)
            {
                if (v.is_private)
                {
                    n++;
                }
            });
    return n;
}

Variable* Reader::getPrivateVariable(const size_t id)
{
    size_t n = 0;
    for (auto& variable : variables)
    {
        if (variable.is_private)
        {
            if (id == n)
            {
                return &variable;
            }
            n++;
        }
    }
    return nullptr;
}

size_t Reader::getPublicVariableCount() const
{
    size_t n = 0;
    std::for_each(
            variables.begin(),
            variables.end(),
            [&n](const Variable& v)
            {
                if (!v.is_private)
                {
                    n++;
                }
            });
    return n;
}

Variable* Reader::getPublicVariable(const size_t id)
{
    size_t n = 0;
    for (auto& variable : variables)
    {
        if (!variable.is_private)
        {
            if (id == n)
            {
                return &variable;
            }
            n++;
        }
    }
    return nullptr;
}

size_t Reader::getImportCount() const
{
    return imports.size();
}

Import* Reader::getImport(const size_t id)
{
    if (id < imports.size())
    {
        return &imports[id];
    }

    return nullptr;
}

size_t Reader::getStateCount() const
{
    return states.size();
}

State* Reader::getState(size_t id)
{
    if (id < states.size())
    {
        return &states[id];
    }
    return nullptr;
}

State* Reader::getStateById(const StateId id)
{
    for (auto& state : states)
    {
        if (id == state.id)
        {
            return &state;
        }
    }
    return nullptr;
}

// public
size_t Reader::getInEventCount() const
{
    size_t n = 0;
    std::for_each(
            events.begin(),
            events.end(),
            [&n](const Event& e)
            {
                if (!e.is_time_event && EventDirection::Incoming == e.direction)
                {
                    n++;
                }
            });
    return n;
}

Event* Reader::getInEvent(size_t id)
{
    size_t n = 0;
    for (auto& e : events)
    {
        if (!e.is_time_event && EventDirection::Incoming == e.direction)
        {
            if (id == n)
            {
                return &e;
            }
            n++;
        }
    }
    return nullptr;
}

Event* Reader::findEvent(const std::string& name)
{
    for (auto& event : events)
    {
        if (event.name == name)
        {
            return &event;
        }
    }
    return nullptr;
}

size_t Reader::getInternalEventCount() const
{
    size_t n = 0;
    std::for_each(
            events.begin(),
            events.end(),
            [&n](const Event& e)
            {
                if (!e.is_time_event && EventDirection::Internal == e.direction)
                {
                    n++;
                }
            });
    return n;
}

Event* Reader::getInternalEvent(size_t id)
{
    size_t n = 0;
    for (auto& e : events)
    {
        if (!e.is_time_event && EventDirection::Internal == e.direction)
        {
            if (id == n)
            {
                return &e;
            }
            n++;
        }
    }
    return nullptr;
}

size_t Reader::getTimeEventCount() const
{
    size_t n = 0;
    std::for_each(
            events.begin(),
            events.end(),
            [&n](const Event& e)
            {
                if (e.is_time_event)
                {
                    n++;
                }
            });
    return n;
}

Event* Reader::getTimeEvent(const size_t id)
{
    size_t n = 0;
    for (auto& e : events)
    {
        if (e.is_time_event)
        {
            if (id == n)
            {
                return &e;
            }
            n++;
        }
    }
    return nullptr;
}

size_t Reader::getOutEventCount() const
{
    size_t n = 0;
    std::for_each(
            events.begin(),
            events.end(),
            [&n](const Event& e)
            {
                if (!e.is_time_event && EventDirection::Outgoing == e.direction)
                {
                    n++;
                }
            });
    return n;
}

Event* Reader::getOutEvent(size_t id)
{
    size_t n = 0;
    for (auto& e : events)
    {
        if (!e.is_time_event && EventDirection::Outgoing == e.direction)
        {
            if (id == n)
            {
                return &e;
            }
            n++;
        }
    }
    return nullptr;
}

size_t Reader::getTransitionCountFromStateId(StateId id) const
{
    size_t n = 0;
    for (const auto& t : transitions)
    {
        if (id == t.state_a)
        {
            n++;
        }
    }
    return n;
}

Transition* Reader::getTransitionFrom(StateId id, size_t tr)
{
    size_t n = 0;
    for (auto& t : transitions)
    {
        const auto tr_state_id = t.state_a;
        if (tr_state_id == id)
        {
            if (tr == n)
            {
                return &t;
            }
            else
            {
                n++;
            }
        }
    }
    return nullptr;
}

size_t Reader::getDeclCount(StateId state_id, Declaration type) const
{
    size_t n = 0;
    std::for_each(
            state_declarations.begin(),
            state_declarations.end(),
            [&state_id, &type, &n](const StateDeclaration& d)
            {
                if (state_id == d.state_id && type == d.type)
                {
                    n++;
                }
            });
    return n;
}

StateDeclaration* Reader::getDeclFromStateId(StateId state_id, Declaration type, const size_t id)
{
    size_t n = 0;
    for (auto& d : state_declarations)
    {
        if (state_id == d.state_id && type == d.type)
        {
            if (id == n)
            {
                return &d;
            }
            n++;
        }
    }
    return nullptr;
}

bool Reader::is_tr_arrow(const std::string& token)
{
    return ('-' == token.front()) && ('>' == token.back());
}

void Reader::collect_states()
{
    in.clear();
    in.seekg(0);

    std::vector<StateId> parentNesting {};
    StateId              parentState {};

    std::string str {};

    auto is_uml    = false;
    auto is_header = false;
    auto is_footer = false;

    while (std::getline(in, str))
    {
        if (!is_uml && ("@startuml" == str))
        {
            // start parsing
            is_uml = true;
        }
        else if (is_uml && ("@enduml" == str))
        {
            // end parsing
            is_uml = false;
        }
        else if (is_uml)
        {
            this->add_uml_line(str);

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
                const auto tokens = tokenize(str);

                if (!tokens.empty())
                {
                    if (("model" == tokens[0]) && (2 == tokens.size()))
                    {
                        model_name = static_cast<char>(std::toupper(tokens[1][0]));
                        model_name.append(tokens[1].substr(1));

                        if (verbose)
                        {
                            std::cout << "Model name detected: " << model_name << std::endl;
                        }
                    }
                    else if (("import" == tokens[0]) && (3 <= tokens.size()))
                    {
                        Import newImport {};
                        if (("global" == tokens[1]) && (4 == tokens.size()))
                        {
                            newImport.is_global = true;
                            newImport.name      = tokens[3];
                        }
                        else
                        {
                            newImport.is_global = false;
                            newImport.name      = tokens[2];
                        }
                        add_import(newImport);
                    }
                    else if ((("private" == tokens[0]) || ("public" == tokens[0])) && (5 <= tokens.size()))
                    {
                        Variable newVariable {};
                        newVariable.is_private = ("private" == tokens[0]);
                        newVariable.name       = tokens[2];
                        newVariable.type       = tokens[4];
                        if (7 == tokens.size())
                        {
                            newVariable.specific_initial_value = true;
                            newVariable.initial_value          = tokens[6];
                        }
                        else
                        {
                            newVariable.specific_initial_value = false;
                            newVariable.initial_value          = "";
                        }
                        add_variable(newVariable);
                    }
                    else if (
                            (("in" == tokens[0]) || ("out" == tokens[0])) && ("event" == tokens[1])
                            && (3 <= tokens.size()))
                    {
                        Event newEvent {};
                        newEvent.name = tokens[2];
                        // time config
                        newEvent.is_time_event  = false;
                        newEvent.expire_time_ms = 0;
                        newEvent.is_periodic    = false;

                        if (5 == tokens.size())
                        {
                            newEvent.require_parameter = true;
                            newEvent.parameter_type    = tokens[4];
                        }
                        else
                        {
                            newEvent.require_parameter = false;
                            newEvent.parameter_type    = "";
                        }

                        if ("in" == tokens[0])
                        {
                            newEvent.direction = EventDirection::Incoming;
                        }
                        else
                        {
                            newEvent.direction = EventDirection::Outgoing;
                        }

                        add_event(newEvent);
                    }
                    else if (("event" == tokens[0]) && (2 <= tokens.size()))
                    {
                        Event newEvent {};
                        newEvent.name = tokens[1];
                        // time config
                        newEvent.is_time_event  = false;
                        newEvent.expire_time_ms = 0;
                        newEvent.is_periodic    = false;

                        if (4 == tokens.size())
                        {
                            newEvent.require_parameter = true;
                            newEvent.parameter_type    = tokens[3];
                        }
                        else
                        {
                            newEvent.require_parameter = false;
                            newEvent.parameter_type    = "";
                        }

                        newEvent.direction = EventDirection::Internal;
                        add_event(newEvent);
                    }
                }
            }
            else
            {
                // parse line
                const auto   tokens    = tokenize(str);
                const size_t numTokens = tokens.size();
                if (0 < numTokens)
                {
                    // =========================================================
                    // STATE DEFINITION # state X Y
                    // =========================================================
                    if (("state" == tokens[0]) && (1 < numTokens))
                    {
                        // define a state
                        State state {};
                        state.name      = tokens[1];
                        state.parent    = parentState;
                        state.is_choice = false;

                        bool setNewParent = false;
                        if (2 < numTokens)
                        {
                            // check for special
                            if ("<<choice>>" == tokens[2])
                            {
                                state.is_choice = true;
                            }
                            else if ("{" == tokens[2])
                            {
                                // parent nesting
                                setNewParent = true;
                            }
                        }

                        const size_t id = add_state(state);
                        if (setNewParent)
                        {
                            if (0 != parentState)
                            {
                                parentNesting.push_back(parentState);
                            }
                            parentState = id;
                        }
                    }

                    // =========================================================
                    // STATE TRANSITION # S1 -> S2 : X Y
                    // If X is a guard [X] then [Y] is considered a part of this
                    // If X is an event X then [Y] is considered its guard
                    // =========================================================
                    else if ((2 < numTokens) && (is_tr_arrow(tokens[1])))
                    {
                        State A {};
                        A.name      = "[*]" == tokens[0] ? "initial" : tokens[0];
                        A.is_choice = false;  // relies on already defined state.
                        A.parent    = parentState;

                        State B {};
                        B.name      = "[*]" == tokens[2] ? "final" : tokens[2];
                        B.is_choice = false;  // see above.
                        B.parent    = parentState;

                        const auto idA = add_state(A);
                        const auto idB = add_state(B);

                        Event ev {};
                        ev.name              = "null";
                        ev.direction         = EventDirection::Incoming;
                        ev.parameter_type    = "";
                        ev.require_parameter = false;
                        ev.is_time_event     = false;
                        ev.expire_time_ms    = 0;
                        ev.is_periodic       = false;

                        Transition tr {};
                        tr.state_a   = idA;
                        tr.state_b   = idB;
                        tr.has_guard = false;
                        tr.guard     = "";

                        if ((4 < numTokens) && (":" == tokens[3]))
                        {
                            if ('[' == tokens[4].front())
                            {
                                // guard only transition.
                                tr.has_guard         = true;
                                std::string guardStr = tokens[4];
                                for (size_t i = 5; i < tokens.size(); i++)
                                {
                                    guardStr += " " + tokens[i];
                                }
                                tr.guard = guardStr.substr(1, guardStr.length() - 2);
                            }
                            else
                            {
                                // check for timed event
                                if (("after" == tokens[4]) || ("every" == tokens[4]))
                                {
                                    // S1 -> S2 : after X u [Y]
                                    // 0  1  2  3 4     5 6 7 - index
                                    // 1  2  3  4 5     6 7 8 - count
                                    ev.is_time_event = true;
                                    ev.name          = A.name + "_" + tokens[4] + "_";
                                    // append time unit to time event name
                                    for (size_t i = 5; i < std::min(tokens.size(), (size_t)7); i++)
                                    {
                                        ev.name += tokens[i];
                                    }

                                    if (6 < numTokens)
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

                                        ev.expire_time_ms = multiplier * ((size_t)std::stoul(tokens[5]));

                                        if ((7 < numTokens) && ('[' == tokens[7].front()))
                                        {
                                            // guard on transition
                                            tr.has_guard         = true;
                                            std::string guardStr = tokens[7];
                                            // get remaining guard string
                                            for (size_t i = 8; i < tokens.size(); i++)
                                            {
                                                guardStr += " " + tokens[i];
                                            }
                                            tr.guard = guardStr.substr(1, guardStr.length() - 2);
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
                                    ev.name = tokens[4];
                                    if ((5 < numTokens) && ('[' == tokens[5].front()))
                                    {
                                        // guard on transition.
                                        tr.has_guard         = true;
                                        std::string guardStr = tokens[5];
                                        for (size_t i = 6; i < tokens.size(); i++)
                                        {
                                            guardStr += " " + tokens[i];
                                        }
                                        tr.guard = guardStr.substr(1, guardStr.length() - 2);
                                    }
                                }
                            }
                        }

                        // add the event
                        tr.event = add_event(ev);

                        // add event to transition and add transition.
                        add_transition(tr);
                    }

                    // =========================================================
                    // STATE ACTION # S : T / X
                    // T is entry/exit/oncycle and X is the action for state S.
                    // Check if X contains a 'raise' keyword followed by Y, then
                    // Y shall be added to the outgoing event list.
                    // =========================================================
                    else if ((2 < numTokens) && (":" == tokens[1]))
                    {
                        // action
                        StateId id = 0;
                        for (auto& state : this->states)
                        {
                            if (tokens[0] == state.name)
                            {
                                id = state.id;
                                break;
                            }
                        }

                        if (0 != id)
                        {
                            if ((3 < numTokens) && ("/" == tokens[3]))
                            {
                                StateDeclaration d {};
                                d.state_id = id;

                                bool hasType = true;
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
                                    hasType = false;
                                }
                                if (hasType)
                                {
                                    // look for any 'raise' stuff
                                    size_t q = 4;
                                    while (q < (tokens.size() - 1))
                                    {
                                        if ("raise" == tokens[q])
                                        {
                                            Event ev {};
                                            ev.name              = tokens[q + 1];
                                            ev.direction         = EventDirection::Internal;  // assume default
                                            ev.require_parameter = false;
                                            ev.parameter_type    = "";
                                            ev.is_time_event     = false;
                                            ev.is_periodic       = false;
                                            ev.expire_time_ms    = 0;
                                            add_event(ev);
                                        }
                                        q++;
                                    }

                                    std::string declStr = tokens[4];
                                    for (size_t i = 5; i < tokens.size(); i++)
                                    {
                                        declStr += " " + tokens[i];
                                    }
                                    d.declaration = declStr;
                                    add_declaration(d);
                                }
                            }
                            else
                            {
                                // comment
                                StateDeclaration d {};
                                d.state_id = id;
                                d.type     = Declaration::Comment;

                                std::string declStr = tokens[2];
                                for (size_t i = 3; i < tokens.size(); i++)
                                {
                                    declStr += " " + tokens[i];
                                }
                                d.declaration = declStr;
                                add_declaration(d);
                            }
                        }
                    }

                    // =========================================================
                    // CLOSE STATE (parent)
                    // =========================================================
                    else if ("}" == tokens[0])
                    {
                        // pop back parent nesting
                        if (!parentNesting.empty())
                        {
                            parentState = parentNesting[parentNesting.size() - 1];
                            parentNesting.pop_back();
                        }
                        else
                        {
                            parentState = 0;
                        }
                    }
                }
            }
        }
    }
}

StateId Reader::add_state(State newState)
{
    static StateId id {};
    StateId        newId {};

    // look for duplicate
    bool isFound = false;
    for (auto& state : this->states)
    {
        if (("initial" == newState.name) || ("final" == newState.name))
        {
            // check if this exact one exists already
            if ((newState.name == state.name) && (newState.parent == state.parent))
            {
                isFound = true;
                newId   = state.id;
                break;
            }
        }
        else
        {
            // require unique names
            if (newState.name == state.name)
            {
                isFound = true;
                newId   = state.id;
                break;  // for
            }
        }
    }

    if (!isFound)
    {
        // new state
        id++;
        newState.id = id;
        states.push_back(newState);
        newId = newState.id;

        if (verbose)
        {
            std::cout << "NEW STATE: " << newState.name << ", id = " << newState.id << ", parent = " << newState.parent
                      << std::endl;
        }
    }

    return newId;
}

Event Reader::add_event(const Event& newEvent)
{
    for (auto& event : this->events)
    {
        if (newEvent.name == event.name)
        {
            std::cout << "Duplicate entry found for " << newEvent.name << std::endl;
            return event;
        }
    }

    // new event
    events.push_back(newEvent);

    if (verbose)
    {
        std::string type = newEvent.is_time_event                             ? "time"
                           : (EventDirection::Incoming == newEvent.direction) ? "incoming"
                           : (EventDirection::Internal == newEvent.direction) ? "internal"
                                                                              : "outgoing";

        std::cout << "Added new (" << type << ") event " << newEvent.name << std::endl;
    }

    return events.back();
}

void Reader::add_transition(const Transition& newTransition)
{
    transitions.push_back(newTransition);

    if (verbose)
    {
        auto A = getStateById(newTransition.state_a);
        auto B = getStateById(newTransition.state_b);

        std::string stateA    = nullptr == A ? "null" : A->name;
        std::string stateB    = nullptr == B ? "null" : B->name;
        std::string guardDesc = newTransition.has_guard ? (" with guard [" + newTransition.guard + "]") : "";

        std::cout << "Added transition " << stateA << " --> " << stateB << " on event " << newTransition.event.name
                  << guardDesc << std::endl;
    }
}

void Reader::add_declaration(const StateDeclaration& newDecl)
{
    state_declarations.push_back(newDecl);

    if (verbose)
    {
        auto st = getStateById(newDecl.state_id);

        std::string name = nullptr == st ? "null" : st->name;
        std::string type = "null";

        if (Declaration::Entry == newDecl.type)
        {
            type = "Entry";
        }
        else if (Declaration::Exit == newDecl.type)
        {
            type = "Exit";
        }
        else if (Declaration::OnCycle == newDecl.type)
        {
            type = "OnCycle";
        }

        std::cout << "Wrote " << type << " declaration for state " << name << std::endl;
    }
}

void Reader::add_variable(const Variable& newVar)
{
    variables.push_back(newVar);

    if (verbose)
    {
        std::cout << "Found variable " << newVar.type << " " << newVar.name;

        if (newVar.specific_initial_value)
        {
            std::cout << " = " << newVar.initial_value << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
    }
}

void Reader::add_import(const Import& newImp)
{
    imports.push_back(newImp);

    if (verbose)
    {
        std::cout << "Found import ";

        if (newImp.is_global)
        {
            std::cout << "<" << newImp.name << ">" << std::endl;
        }
        else
        {
            std::cout << "\"" << newImp.name << "\"" << std::endl;
        }
    }
}

void Reader::add_uml_line(const std::string& line)
{
    uml.push_back(line);
}

std::vector<std::string> Reader::tokenize(const std::string& str)
{
    std::vector<std::string> tokens {};
    std::string              tmp {};
    std::istringstream       iss(str);
    while (iss >> tmp)
    {
        tokens.push_back(tmp);
    }
    return (tokens);
}
