/** @file
 *  @brief Handles styling of the generated code.
 */

#pragma once

#include <string>
#include "reader.hpp"

class Style
{
private:
    bool use_simple_names;
    Reader& reader;
    std::string get_state_base_decl(const State* state);
    static std::string convert_snake_case(const std::string& str);
    static void transform_lower(std::string& str);
public:
    explicit Style(Reader& reader);
    ~Style() = default;

    void set_simple_names(bool enable);

    static std::string get_top_run_cycle();
    std::string get_state_run_cycle(const State* state);
    std::string get_state_entry(const State* state);
    std::string get_state_exit(const State* state);
    std::string get_state_name(const State* state);
    std::string get_state_name_pure(const State* state);
    static std::string get_state_type();
    static std::string get_event_raise(const Event* event);
    static std::string get_event_raise(const std::string& eventName);
    static std::string get_event_name(const Event* event);
    static std::string get_time_tick();
    static std::string get_event_is_raised(const Event* event);
    static std::string get_event_value(const Event* event);
    static std::string get_variable_name(const Variable* var);
    static std::string get_trace_entry();
    static std::string get_trace_exit();
};
