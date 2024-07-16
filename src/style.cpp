/** @file
 *  @brief Implementation of the styling class.
 */

#include "../include/style.hpp"
#include <algorithm>
#include <cctype>
#include <string>

Style::Style(Reader& reader) : reader(reader), use_simple_names(false)
{
}

void Style::set_simple_names(bool enable)
{
    use_simple_names = enable;
}

std::string Style::get_state_base_decl(const State* state)
{
    std::string decl_base {};

    if (!use_simple_names)
    {
        auto parent = reader.getStateById(state->parent);
        if (nullptr != parent)
        {
            decl_base = get_state_base_decl(parent) + "_";
        }
    }

    decl_base += state->name;
    decl_base = convert_snake_case(decl_base);

    return (decl_base);
}

std::string Style::convert_snake_case(const std::string& str)
{
    std::string snake_case {};
    for (unsigned char ch : str)
    {
        if (std::isupper(ch))
        {
            if (!snake_case.empty())
            {
                snake_case += '_';
            }
            snake_case += std::tolower(ch);
        }
        else
        {
            snake_case += ch;
        }
    }
    return snake_case;
}

void Style::transform_lower(std::string& str)
{
    /* Convert string to lower case, and any CamelCase to snake_case. */
    std::transform(
            str.begin(),
            str.end(),
            str.begin(),
            [](unsigned char c)
            {
                return std::tolower(c);
            });
}

std::string Style::get_top_run_cycle()
{
    return "run_cycle";
}

std::string Style::get_state_run_cycle(const State* state)
{
    return "state_" + convert_snake_case(get_state_base_decl(state)) + "_react";
}

std::string Style::get_state_entry(const State* state)
{
    return "state_" + convert_snake_case(get_state_base_decl(state)) + "_entry_action";
}

std::string Style::get_state_exit(const State* state)
{
    return "state_" + convert_snake_case(get_state_base_decl(state)) + "_exit_action";
}

std::string Style::get_state_name(const State* state)
{
    return get_state_type() + "::" + convert_snake_case(get_state_base_decl(state));
}

std::string Style::get_state_name_pure(const State* state)
{
    return get_state_base_decl(state);
}

std::string Style::get_state_type()
{
    return "State";
}

std::string Style::get_event_raise(const Event* event)
{
    return "raise_" + convert_snake_case(event->name);
}

std::string Style::get_event_raise(const std::string& eventName)
{
    return "raise_" + convert_snake_case(eventName);
}

std::string Style::get_event_name(const Event* event)
{
    return convert_snake_case(event->name);
}

std::string Style::get_time_tick()
{
    return "time_tick";
}

std::string Style::get_event_is_raised(const Event* event)
{
    return "is_" + convert_snake_case(event->name) + "_raised";
}

std::string Style::get_event_value(const Event* event)
{
    return convert_snake_case(event->name) + "_value";
}

std::string Style::get_variable_name(const Variable* var)
{
    return convert_snake_case(var->name);
}

std::string Style::get_trace_entry()
{
    return "trace_state_enter";
}

std::string Style::get_trace_exit()
{
    return "trace_state_exit";
}
