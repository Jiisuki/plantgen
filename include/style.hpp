/** @file
 *  @brief Handles styling of the generated code.
 */

#pragma once

#include "uml_parser.hpp"
#include <string>

class Style
{
  private:
    bool                   use_simple_names;
    UML::Parser::SharedPtr parser;
    std::string            get_state_base_decl(UML::StatePtr state);
    static std::string     convert_snake_case(const std::string& str);
    static void            transform_lower(std::string& str);

  public:
    explicit Style(UML::Parser::SharedPtr reader);
    ~Style() = default;

    using SharedPtr = std::shared_ptr<Style>;

    void set_simple_names(bool enable);

    static std::string get_top_run_cycle();
    std::string        get_state_run_cycle(UML::StatePtr state);
    std::string        get_state_entry(UML::StatePtr state);
    std::string        get_state_exit(UML::StatePtr state);
    std::string        get_state_name(UML::StatePtr state);
    std::string        get_state_name_pure(UML::StatePtr state);
    static std::string get_state_type();
    static std::string get_event_raise(UML::EventPtr event);
    static std::string get_event_raise(const std::string& eventName);
    static std::string get_event_name(UML::EventPtr event);
    static std::string get_time_tick();
    static std::string get_event_is_raised(UML::EventPtr event);
    static std::string get_event_value(UML::EventPtr event);
    static std::string get_variable_name(UML::VariablePtr var);
    static std::string get_trace_entry();
    static std::string get_trace_exit();
};
