/** @file
 *  @brief Implementation of the reader class.
 */

#include <algorithm>
#include <caca++.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "uml_parser.hpp"

namespace UML
{
    std::string Parser::get_model_name() const
    {
        return model_name;
    }

    size_t Parser::get_uml_line_count() const
    {
        return uml.size();
    }

    std::string Parser::get_uml_line(const size_t i) const
    {
        return (i < uml.size()) ? uml[i] : "";
    }

    Variable::SharedPtr Parser::get_public_variable(const std::string& name)
    {
        for (auto& variable : variables)
        {
            if ((variable->name == name) && !variable->is_private)
            {
                return variable;
            }
        }

        throw std::runtime_error("Variable not found: " + name);
    }

    std::vector<Variable::SharedPtr> Parser::get_public_variables() const
    {
        std::vector<Variable::SharedPtr> public_variables {};
        std::for_each(
                variables.begin(),
                variables.end(),
                [&public_variables](const Variable::SharedPtr& variable)
                {
                    if (!variable->is_private)
                    {
                        public_variables.push_back(variable);
                    }
                });

        return public_variables;
    }

    Variable::SharedPtr Parser::get_private_variable(const std::string& name)
    {
        for (auto& variable : variables)
        {
            if ((variable->name == name) && variable->is_private)
            {
                return variable;
            }
        }

        throw std::runtime_error("Variable not found: " + name);
    }

    std::vector<Variable::SharedPtr> Parser::get_private_variables() const
    {
        std::vector<Variable::SharedPtr> private_variables {};
        std::for_each(
                variables.begin(),
                variables.end(),
                [&private_variables](const Variable::SharedPtr& variable)
                {
                    if (variable->is_private)
                    {
                        private_variables.push_back(variable);
                    }
                });

        return private_variables;
    }

    Import::SharedPtr Parser::get_import(const std::string& name)
    {
        for (auto& import : imports)
        {
            if (import->name == name)
            {
                return import;
            }
        }

        throw std::runtime_error("Import not found: " + name);
    }

    std::vector<Import::SharedPtr> Parser::get_imports() const
    {
        return imports;
    }

    State::Definition::SharedPtr Parser::get_state(const std::string& name)
    {
        for (auto& state : states)
        {
            if (state->name == name)
            {
                return state;
            }
        }

        throw std::runtime_error("State not found: " + name);
    }

    std::vector<State::Definition::SharedPtr> Parser::get_states() const
    {
        return states;
    }

    Event::Definition::SharedPtr Parser::get_in_event(const std::string& name)
    {
        for (auto& event : events)
        {
            if ((Event::Type::Incoming == event->type) && (event->name == name))
            {
                return event;
            }
        }

        throw std::runtime_error("In event not found: " + name);
    }

    std::vector<Event::Definition::SharedPtr> Parser::get_in_events() const
    {
        std::vector<Event::Definition::SharedPtr> in_events {};
        std::for_each(
                events.begin(),
                events.end(),
                [&in_events](const Event::Definition::SharedPtr& event)
                {
                    if (Event::Type::Incoming == event->type)
                    {
                        in_events.push_back(event);
                    }
                });

        return in_events;
    }

    Event::Definition::SharedPtr Parser::get_out_event(const std::string& name)
    {
        for (auto& event : events)
        {
            if ((Event::Type::Outgoing == event->type) && (event->name == name))
            {
                return event;
            }
        }

        throw std::runtime_error("Out event not found: " + name);
    }

    std::vector<Event::Definition::SharedPtr> Parser::get_out_events() const
    {
        std::vector<Event::Definition::SharedPtr> out_events {};
        std::for_each(
                events.begin(),
                events.end(),
                [&out_events](const Event::Definition::SharedPtr& event)
                {
                    if (Event::Type::Outgoing == event->type)
                    {
                        out_events.push_back(event);
                    }
                });

        return out_events;
    }

    Event::Definition::SharedPtr Parser::get_internal_event(const std::string& name)
    {
        for (auto& event : events)
        {
            if ((Event::Type::Internal == event->type) && (event->name == name))
            {
                return event;
            }
        }

        throw std::runtime_error("Internal event not found: " + name);
    }

    std::vector<Event::Definition::SharedPtr> Parser::get_internal_events() const
    {
        std::vector<Event::Definition::SharedPtr> internal_events {};
        std::for_each(
                events.begin(),
                events.end(),
                [&internal_events](const Event::Definition::SharedPtr& event)
                {
                    if (Event::Type::Internal == event->type)
                    {
                        internal_events.push_back(event);
                    }
                });

        return internal_events;
    }

    Event::Definition::SharedPtr Parser::get_time_event(const std::string& name)
    {
        for (auto& event : events)
        {
            if ((Event::Type::Time == event->type) && (event->name == name))
            {
                return event;
            }
        }

        throw std::runtime_error("Time event not found: " + name);
    }

    std::vector<Event::Definition::SharedPtr> Parser::get_time_events() const
    {
        std::vector<Event::Definition::SharedPtr> time_events {};
        std::for_each(
                events.begin(),
                events.end(),
                [&time_events](const Event::Definition::SharedPtr& event)
                {
                    if (Event::Type::Time == event->type)
                    {
                        time_events.push_back(event);
                    }
                });

        return time_events;
    }

    std::vector<Transition::SharedPtr> Parser::get_transitions_from_state(State::Definition::SharedPtr state)
    {
        std::vector<Transition::SharedPtr> transitions_from_state {};
        for (auto& transition : this->transitions)
        {
            if (transition->from_state->name == state->name)
            {
                transitions_from_state.push_back(transition);
            }
        }

        return transitions_from_state;
    }

    bool Parser::is_tr_arrow(const std::string& token)
    {
        return ('-' == token.front()) && ('>' == token.back());
    }

    std::vector<std::string> Parser::tokenize(const std::string& str)
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

    void Parser::parse(const std::string& filename)
    {
        std::ifstream in(filename);

        if (!in.is_open())
        {
            throw std::runtime_error("Could not open file: " + filename);
        }

        in.clear();
        in.seekg(0, std::ios::beg);

        if (!in.good())
        {
            throw std::runtime_error("Could not read file: " + filename);
        }

        std::vector<std::string> parent_nesting {};
        std::string              parent_state {};

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
                uml.push_back(str);

                if ("header" == str)
                {
                    // start header parsing
                    is_header = true;
                }
                else if ("footer" == str)
                {
                    // start footer parsing
                    is_footer = true;
                }
                else if ("endheader" == str)
                {
                    // end header parsing
                    is_header = false;
                }
                else if ("endfooter" == str)
                {
                    // end footer parsing
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

                            std::cout << "Model name detected: " << model_name << std::endl;
                        }
                        else if (("import" == tokens[0]) && (3 <= tokens.size()))
                        {
                            auto import = std::make_shared<Import>();

                            if (("global" == tokens[1]) && (4 == tokens.size()))
                            {
                                import->is_global = true;
                                import->name      = tokens[3];
                            }
                            else
                            {
                                import->is_global = false;
                                import->name      = tokens[2];
                            }

                            add_import(import);
                        }
                        else if ((("private" == tokens[0]) || ("public" == tokens[0])) && (5 <= tokens.size()))
                        {
                            auto variable = std::make_shared<Variable>();

                            variable->is_private = ("private" == tokens[0]);
                            variable->name       = tokens[2];
                            variable->type       = tokens[4];

                            if (7 == tokens.size())
                            {
                                variable->initial_value = tokens[6];
                            }
                            else
                            {
                                variable->initial_value = "";
                            }

                            add_variable(variable);
                        }
                        else if (
                                (("in" == tokens[0]) || ("out" == tokens[0])) && ("event" == tokens[1])
                                && (3 <= tokens.size()))
                        {
                            auto event  = std::make_shared<Event::Definition>();
                            event->name = tokens[2];

                            if (5 == tokens.size())
                            {
                                event->require_parameter = true;
                                event->parameter_type    = tokens[4];
                            }
                            else
                            {
                                event->require_parameter = false;
                                event->parameter_type    = "";
                            }

                            if ("in" == tokens[0])
                            {
                                event->type = Event::Type::Incoming;
                            }
                            else
                            {
                                event->type = Event::Type::Outgoing;
                            }

                            add_event(event);
                        }
                        else if (("event" == tokens[0]) && (2 <= tokens.size()))
                        {
                            auto event  = std::make_shared<Event::Definition>();
                            event->name = tokens[1];
                            event->type = Event::Type::Internal;

                            if (4 == tokens.size())
                            {
                                event->require_parameter = true;
                                event->parameter_type    = tokens[3];
                            }
                            else
                            {
                                event->require_parameter = false;
                                event->parameter_type    = "";
                            }

                            add_event(event);
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
                            auto state = std::make_shared<State::Definition>();

                            state->name      = tokens[1];
                            state->parent    = parent_state;
                            state->is_choice = false;

                            bool set_new_parent = false;
                            if (2 < numTokens)
                            {
                                // check for special
                                if ("<<choice>>" == tokens[2])
                                {
                                    state->is_choice = true;
                                }
                                else if ("{" == tokens[2])
                                {
                                    // parent nesting
                                    set_new_parent = true;
                                }
                            }

                            const auto ptr = add_state(state);

                            if (set_new_parent)
                            {
                                if (!parent_state.empty())
                                {
                                    parent_nesting.push_back(parent_state);
                                }
                                parent_state = ptr->name;
                            }
                        }

                        // =========================================================
                        // STATE TRANSITION # S1 -> S2 : X Y
                        // If X is a guard [X] then [Y] is considered a part of this
                        // If X is an event X then [Y] is considered its guard
                        // =========================================================
                        else if ((2 < numTokens) && (is_tr_arrow(tokens[1])))
                        {
                            auto A       = std::make_shared<State::Definition>();
                            A->name      = "[*]" == tokens[0] ? "initial" : tokens[0];
                            A->is_choice = false;  // relies on already defined state.
                            A->parent    = parent_state;

                            auto B       = std::make_shared<State::Definition>();
                            B->name      = "[*]" == tokens[2] ? "final" : tokens[2];
                            B->is_choice = false;  // see above.
                            B->parent    = parent_state;

                            const auto ptr_a = add_state(A);
                            const auto ptr_b = add_state(B);

                            auto ev               = std::make_shared<Event::Definition>();
                            ev->name              = "null";
                            ev->parameter_type    = "";
                            ev->require_parameter = false;
                            ev->type              = Event::Type::Internal;
                            ev->expire_time_ms    = 0;
                            ev->is_periodic       = false;

                            std::string guard {};

                            if ((4 < numTokens) && (":" == tokens[3]))
                            {
                                if ('[' == tokens[4].front())
                                {
                                    // guard only transition.
                                    std::string guard_str = tokens[4];
                                    for (size_t i = 5; i < tokens.size(); i++)
                                    {
                                        guard_str += " " + tokens[i];
                                    }
                                    guard = guard_str.substr(1, guard_str.length() - 2);
                                }
                                else
                                {
                                    // check for timed event
                                    if (("after" == tokens[4]) || ("every" == tokens[4]))
                                    {
                                        // S1 -> S2 : after X u [Y]
                                        // 0  1  2  3 4     5 6 7 - index
                                        // 1  2  3  4 5     6 7 8 - count
                                        ev->type = Event::Type::Time;
                                        ev->name = ptr_a->name + "_" + tokens[4] + "_";
                                        // append time unit to time event name
                                        for (size_t i = 5; i < std::min(tokens.size(), (size_t)7); i++)
                                        {
                                            ev->name += tokens[i];
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

                                            ev->expire_time_ms = multiplier * ((size_t)std::stoul(tokens[5]));

                                            if ((7 < numTokens) && ('[' == tokens[7].front()))
                                            {
                                                // guard on transition
                                                std::string guard_str = tokens[7];
                                                // get remaining guard string
                                                for (size_t i = 8; i < tokens.size(); i++)
                                                {
                                                    guard_str += " " + tokens[i];
                                                }
                                                guard = guard_str.substr(1, guard_str.length() - 2);
                                            }
                                        }
                                        else
                                        {
                                            throw std::runtime_error("No time specified on time event.");
                                        }
                                    }
                                    else
                                    {
                                        // normal event
                                        ev->name = tokens[4];
                                        if ((5 < numTokens) && ('[' == tokens[5].front()))
                                        {
                                            // guard on transition.
                                            std::string guard_str = tokens[5];
                                            for (size_t i = 6; i < tokens.size(); i++)
                                            {
                                                guard_str += " " + tokens[i];
                                            }
                                            guard = guard_str.substr(1, guard_str.length() - 2);
                                        }
                                    }
                                }
                            }

                            // add the event
                            auto ev_ptr = add_event(ev);
                            auto tr     = std::make_shared<Transition>(ptr_a, ptr_b, ev_ptr, guard);

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
                            auto ptr = get_state(tokens[0]);

                            if (ptr)
                            {
                                auto d = std::make_shared<State::Declaration>();
                                if ((3 < numTokens) && ("/" == tokens[3]))
                                {
                                    if ("entry" == tokens[2])
                                    {
                                        d->type = State::Type::Entry;
                                    }
                                    else if ("exit" == tokens[2])
                                    {
                                        d->type = State::Type::Exit;
                                    }
                                    else if ("oncycle" == tokens[2])
                                    {
                                        d->type = State::Type::OnCycle;
                                    }
                                    else
                                    {
                                        throw std::runtime_error("Unknown action type: " + tokens[2]);
                                    }

                                    // look for any 'raise' stuff
                                    size_t q = 4;
                                    while (q < (tokens.size() - 1))
                                    {
                                        if ("raise" == tokens[q])
                                        {
                                            auto ev = std::make_shared<Event::Definition>();

                                            ev->name              = tokens[q + 1];
                                            ev->type              = Event::Type::Internal;
                                            ev->require_parameter = false;
                                            ev->parameter_type    = "";
                                            ev->is_periodic       = false;
                                            ev->expire_time_ms    = 0;

                                            add_event(ev);
                                        }
                                        q++;
                                    }

                                    std::string decl_str = tokens[4];
                                    for (size_t i = 5; i < tokens.size(); i++)
                                    {
                                        decl_str += " " + tokens[i];
                                    }

                                    d->contents = decl_str;
                                }
                                else
                                {
                                    // comment
                                    d->type = State::Type::Comment;

                                    std::string decl_str = tokens[2];
                                    for (size_t i = 3; i < tokens.size(); i++)
                                    {
                                        decl_str += " " + tokens[i];
                                    }

                                    d->contents = decl_str;
                                }

                                ptr->declarations.push_back(d);
                            }
                        }

                        // =========================================================
                        // CLOSE STATE (parent)
                        // =========================================================
                        else if ("}" == tokens[0])
                        {
                            // pop back parent nesting
                            if (!parent_nesting.empty())
                            {
                                parent_state = parent_nesting.back();
                                parent_nesting.pop_back();
                            }
                            else
                            {
                                parent_state.clear();
                            }
                        }
                    }
                }
            }
        }
    }

    State::Definition::SharedPtr Parser::add_state(State::Definition::SharedPtr state)
    {
        auto it = std::find_if(
                states.begin(),
                states.end(),
                [&state](const State::Definition::SharedPtr& s)
                {
                    if (("initial" == state->name) || ("final" == state->name))
                    {
                        return (s->name == state->name) && (state->parent == s->parent);
                    }
                    else
                    {
                        return s->name == state->name;
                    }
                });

        if (it == states.end())
        {
            std::cout << "New state: " << state->name << ", parent = " << state->parent << std::endl;

            states.push_back(state);
            return state;
        }
        else
        {
            return *it;
        }
    }

    Event::Definition::SharedPtr Parser::add_event(Event::Definition::SharedPtr event)
    {
        auto it = std::find_if(
                events.begin(),
                events.end(),
                [&event](const Event::Definition::SharedPtr& e)
                {
                    return e->name == event->name;
                });

        if (it == events.end())
        {
            std::cout << "New event: " << event->name << ", Type: "
                      << (Event::Type::Incoming == event->type   ? "incoming"
                          : Event::Type::Outgoing == event->type ? "outgoing"
                          : Event::Type::Time == event->type     ? "time"
                                                                 : "internal")
                      << std::endl;

            events.push_back(event);
            return event;
        }

        return *it;
    }

    Transition::SharedPtr Parser::add_transition(Transition::SharedPtr transition)
    {
        auto it = std::find_if(
                transitions.begin(),
                transitions.end(),
                [&transition](const Transition::SharedPtr& t)
                {
                    return (t->from_state->name == transition->from_state->name)
                           && (t->to_state->name == transition->to_state->name)
                           && (t->on_event->name == transition->on_event->name);
                });

        if (it == transitions.end())
        {
            std::cout << "New transition: " << transition->from_state->name << " --> " << transition->to_state->name
                      << " on event " << transition->on_event->name << " with guard '" << transition->guard_expression
                      << "'" << std::endl;

            transitions.push_back(transition);
            return transition;
        }

        return *it;
    }

    Variable::SharedPtr Parser::add_variable(Variable::SharedPtr var)
    {
        auto it = std::find_if(
                variables.begin(),
                variables.end(),
                [&var](const Variable::SharedPtr& v)
                {
                    return (v->name == var->name) && (v->is_private == var->is_private);
                });

        if (it == variables.end())
        {
            std::cout << "New " << (var->is_private ? "private" : "public") << " variable: " << var->name << " = "
                      << (var->initial_value.empty() ? "{}" : var->initial_value) << std::endl;

            variables.push_back(var);
            return var;
        }

        return *it;
    }

    Import::SharedPtr Parser::add_import(Import::SharedPtr imp)
    {
        auto it = std::find_if(
                imports.begin(),
                imports.end(),
                [&imp](const Import::SharedPtr& i)
                {
                    return i->name == imp->name;
                });

        if (it == imports.end())
        {
            std::cout << "New import: " << (imp->is_global ? "<" : "\"") << imp->name << (imp->is_global ? ">" : "\"")
                      << std::endl;

            imports.push_back(imp);
            return imp;
        }

        return *it;
    }
}  // namespace UML