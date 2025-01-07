/** @file
 *  @brief Class for reading a plantuml file.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace UML
{
    namespace State
    {
        enum class Type
        {
            Entry,
            Exit,
            OnCycle,
            Comment,
        };

        struct Declaration
        {
            Type        type;
            std::string contents;

            Declaration() : type(), contents() {}
            ~Declaration() = default;

            using SharedPtr = std::shared_ptr<Declaration>;
        };

        struct Definition
        {
            bool                                is_choice;
            std::string                         name;
            std::string                         parent;
            std::vector<Declaration::SharedPtr> declarations;

            Definition() : is_choice(), name(), parent(), declarations() {}
            ~Definition() = default;

            using SharedPtr = std::shared_ptr<Definition>;

            std::vector<std::string> get_declarations_by_type(Type type)
            {
                std::vector<std::string> out {};
                std::for_each(
                        declarations.begin(),
                        declarations.end(),
                        [&out, type](const auto& decl)
                        {
                            if (type == decl->type)
                            {
                                out.push_back(decl->contents);
                            }
                        });
                return out;
            }
        };

    }  // namespace State

    namespace Event
    {
        enum class Type
        {
            Incoming,
            Outgoing,
            Internal,
            Time,
        };

        struct Definition
        {
            Type        type;
            std::string name;
            bool        require_parameter;
            std::string parameter_type;
            size_t      expire_time_ms;
            bool        is_periodic;

            Definition() : type(), name(), require_parameter(), parameter_type(), expire_time_ms(), is_periodic() {}
            ~Definition() = default;

            using SharedPtr = std::shared_ptr<Definition>;
        };
    }  // namespace Event

    struct Transition
    {
        State::Definition::SharedPtr from_state;
        State::Definition::SharedPtr to_state;
        Event::Definition::SharedPtr on_event;
        std::string                  guard_expression;

        Transition(
                State::Definition::SharedPtr from,
                State::Definition::SharedPtr to,
                Event::Definition::SharedPtr event,
                std::string                  guard = std::string()) :
            from_state(std::move(from)),
            to_state(std::move(to)),
            on_event(std::move(event)),
            guard_expression(std::move(guard))
        {
        }
        ~Transition() = default;

        using SharedPtr = std::shared_ptr<Transition>;
    };

    struct Variable
    {
        std::string name;
        std::string type;
        bool        is_private;
        std::string initial_value;

        Variable() : name(), type(), is_private(), initial_value() {}
        ~Variable() = default;

        using SharedPtr = std::shared_ptr<Variable>;
    };

    struct Import
    {
        bool        is_global;
        std::string name;

        Import() : is_global(), name() {}
        ~Import() = default;

        using SharedPtr = std::shared_ptr<Import>;
    };

    using StatePtr      = State::Definition::SharedPtr;
    using EventPtr      = Event::Definition::SharedPtr;
    using TransitionPtr = Transition::SharedPtr;
    using VariablePtr   = Variable::SharedPtr;
    using ImportPtr     = Import::SharedPtr;

    class Parser
    {
      private:
        std::string              model_name;
        std::vector<std::string> uml;

        std::vector<State::Definition::SharedPtr> states;
        std::vector<Event::Definition::SharedPtr> events;
        std::vector<Transition::SharedPtr>        transitions;
        std::vector<Variable::SharedPtr>          variables;
        std::vector<Import::SharedPtr>            imports;

        State::Definition::SharedPtr add_state(State::Definition::SharedPtr state);
        Event::Definition::SharedPtr add_event(Event::Definition::SharedPtr event);
        Transition::SharedPtr        add_transition(Transition::SharedPtr transition);
        Variable::SharedPtr          add_variable(Variable::SharedPtr var);
        Import::SharedPtr            add_import(Import::SharedPtr imp);

        static bool                     is_tr_arrow(const std::string& token);
        static std::vector<std::string> tokenize(const std::string& str);

      public:
        Parser()  = default;
        ~Parser() = default;

        using SharedPtr = std::shared_ptr<Parser>;

        void parse(const std::string& filename);

        [[nodiscard]] std::string get_model_name() const;

        [[nodiscard]] size_t      get_uml_line_count() const;
        [[nodiscard]] std::string get_uml_line(size_t i) const;

        Variable::SharedPtr          get_public_variable(const std::string& name);
        Variable::SharedPtr          get_private_variable(const std::string& name);
        Import::SharedPtr            get_import(const std::string& name);
        State::Definition::SharedPtr get_state(const std::string& name);
        Event::Definition::SharedPtr get_in_event(const std::string& name);
        Event::Definition::SharedPtr get_out_event(const std::string& name);
        Event::Definition::SharedPtr get_internal_event(const std::string& name);
        Event::Definition::SharedPtr get_time_event(const std::string& name);

        [[nodiscard]] std::vector<Variable::SharedPtr>          get_public_variables() const;
        [[nodiscard]] std::vector<Variable::SharedPtr>          get_private_variables() const;
        [[nodiscard]] std::vector<Import::SharedPtr>            get_imports() const;
        [[nodiscard]] std::vector<State::Definition::SharedPtr> get_states() const;
        [[nodiscard]] std::vector<Event::Definition::SharedPtr> get_in_events() const;
        [[nodiscard]] std::vector<Event::Definition::SharedPtr> get_out_events() const;
        [[nodiscard]] std::vector<Event::Definition::SharedPtr> get_internal_events() const;
        [[nodiscard]] std::vector<Event::Definition::SharedPtr> get_time_events() const;

        std::vector<Transition::SharedPtr> get_transitions_from_state(State::Definition::SharedPtr state);
    };
}  // namespace UML
