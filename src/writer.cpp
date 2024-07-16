/** @file
 *  @brief Implementation of the writer class.
 */

#include "../include/writer.hpp"
#include "../include/reader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

Writer::Writer(const std::string& filename, const std::string& outdir, const WriterConfig& cfg) :
    config(cfg), filename(filename), outdir(outdir), reader(filename, cfg.verbose), styler(reader), indent()
{
}

void Writer::generateCode()
{
    styler.set_simple_names(config.use_simple_names);

    auto model = reader.get_model_name();
    if (!model.empty())
    {
        model[0] = static_cast<char>(std::tolower(model[0]));
    }

    const auto outfile_c = outdir + model + ".cpp";
    const auto outfile_h = outdir + model + ".h";

    if (config.verbose)
    {
        std::cout << "Generating code from '" << filename << "' > '" << outfile_c << "' and '" << outfile_h << "' ..."
                  << std::endl;
    }

    std::ofstream out_c {};
    out_c.open(outfile_c);

    std::ofstream out_h {};
    out_h.open(outfile_h);

    if (!out_c.is_open() || !out_h.is_open())
    {
        error_report("Failed to open output files, does directory exist?", __LINE__);
        return;
    }

    out_h << "/** @file" << std::endl;
    out_h << " *  @brief Interface to the " << reader.get_model_name() << " state machine." << std::endl;
    out_h << " *" << std::endl;
    out_h << " *  @startuml" << std::endl;
    for (size_t i = 0; i < reader.get_uml_line_count(); i++)
    {
        out_h << " *  " << reader.get_uml_line(i) << std::endl;
    }
    out_h << " *  @enduml" << std::endl;
    out_h << " */" << std::endl << std::endl;

    out_h << get_indent() << "#include <cstdint>" << std::endl;
    out_h << get_indent() << "#include <cstddef>" << std::endl;
    out_h << get_indent() << "#include <functional>" << std::endl;
    out_h << get_indent() << "#include <deque>" << std::endl;
    out_h << get_indent() << "#include <string>" << std::endl;

    for (auto i = 0u; i < reader.getImportCount(); i++)
    {
        auto imp = reader.getImport(i);
        out_h << get_indent() << "#include ";
        if (imp->is_global)
        {
            out_h << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            out_h << "\"" << imp->name << "\"" << std::endl;
        }
    }
    out_h << std::endl;

    // setup namespace
    start_namespace(out_h);

    // write all states to .h
    decl_state_list(out_h);

    // write all event types
    decl_event_list(out_h);

    // write all variables
    decl_variable_list(out_h);

    // write tracing callback types
    decl_tracing_callback(out_h);

    // write main declaration
    decl_state_machine(out_h);

    // end namespace
    end_namespace(out_h);

    // write header to .c
    out_c << get_indent() << "#include \"" << model << ".h\"" << std::endl << std::endl;

    for (auto i = 0u; i < reader.getImportCount(); i++)
    {
        auto imp = reader.getImport(i);
        out_c << get_indent() << "#include ";
        if (imp->is_global)
        {
            out_c << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            out_c << "\"" << imp->name << "\"" << std::endl;
        }
    }
    out_c << std::endl;

    // setup namespace
    start_namespace(out_c);

    // find first state on init
    const auto firstState = find_init_state();

    // write init implementation
    impl_init(out_c, firstState);

    // write trace calls wrappers
    impl_trace_calls(out_c);

    // write all raise event functions
    impl_raise_in_event(out_c);
    impl_check_out_event(out_c);
    impl_get_variable(out_c);
    impl_time_tick(out_c);
    impl_top_run_cycle(out_c);
    impl_run_cycle(out_c);
    impl_entry_action(out_c);
    impl_exit_action(out_c);
    impl_raise_out_event(out_c);
    impl_raise_internal_event(out_c);

    // end namespace
    end_namespace(out_c);

    // close streams
    out_c.close();
    out_h.close();
}

void Writer::error_report(const std::string& str, unsigned int line)
{
    std::string  filename   = __FILE__;
    const size_t last_slash = filename.find_last_of('/');
    std::cout << "ERR: " << str << " - " << filename.substr(last_slash + 1) << ": " << line << std::endl;
}

void Writer::increase_indent()
{
    indent++;
}

void Writer::decrease_indent()
{
    if (0 < indent)
    {
        indent--;
    }
}

void Writer::reset_indent()
{
    indent = 0;
}

void Writer::start_namespace(std::ofstream& out)
{
    reset_indent();
    out << "namespace " << reader.get_model_name() << std::endl;
    out << "{" << std::endl;
    increase_indent();
}

void Writer::end_namespace(std::ofstream& out)
{
    reset_indent();
    out << "}" << std::endl << std::endl;
}

void Writer::decl_state_list(std::ofstream& out)
{
    out << get_indent() << "enum class " << Style::get_state_type() << std::endl;
    out << get_indent() << "{" << std::endl;
    increase_indent();

    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if (nullptr != state)
        {
            /* Only write down state on actual states that the machine may stay in. */
            if (("initial" != state->name) && ("final" != state->name) && (!state->is_choice))
            {
                out << get_indent() << styler.get_state_name_pure(state) << "," << std::endl;
            }
        }
    }
    decrease_indent();

    out << get_indent() << "};" << std::endl << std::endl;
}

void Writer::decl_event_list(std::ofstream& out)
{
    const auto n_in_events       = reader.getInEventCount();
    const auto n_out_events      = reader.getOutEventCount();
    const auto n_time_events     = reader.getTimeEventCount();
    const auto n_internal_events = reader.getInternalEventCount();

    // create an enum of all out-event names, out-events are on a separate queue since these are cleared by the user.
    if (0 < n_out_events)
    {
        out << get_indent() << "enum class " << reader.get_model_name() << "_OutEventId" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        for (std::size_t i = 0; i < n_out_events; i++)
        {
            auto ev = reader.getOutEvent(i);
            if ((nullptr != ev) && ("null" != ev->name))
            {
                out << get_indent() << ev->name << "," << std::endl;
            }
        }
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;

        // create a union of any event data possible.
        std::vector<std::pair<std::string, std::string>> paramData {};
        for (auto i = 0u; i < n_out_events; i++)
        {
            auto ev = reader.getOutEvent(i);
            if ((nullptr != ev) && ev->require_parameter && ("null" != ev->name))
            {
                paramData.emplace_back(ev->parameter_type, ev->name);
            }
        }

        if (!paramData.empty())
        {
            out << get_indent() << "union " << reader.get_model_name() << "_OutEventData" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            for (const auto& x : paramData)
            {
                out << get_indent() << x.first << " " << x.second << ";" << std::endl;
            }
            out << get_indent() << reader.get_model_name() << "_OutEventData() = default;" << std::endl;  //: ";
            out << get_indent() << "~" << reader.get_model_name() << "_OutEventData() = default;" << std::endl;
            decrease_indent();

            out << get_indent() << "};" << std::endl << std::endl;
        }

        // create struct containing the in-event.
        out << get_indent() << "struct " << reader.get_model_name() << "_OutEvent" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << reader.get_model_name() << "_OutEventId id;" << std::endl;
        if (!paramData.empty())
        {
            out << get_indent() << reader.get_model_name() << "_OutEventData parameter;" << std::endl;
        }
        out << get_indent() << reader.get_model_name() << "_OutEvent() = default;" << std::endl;
        out << get_indent() << "~" << reader.get_model_name() << "_OutEvent() = default;" << std::endl;
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;
    }

    if (0 < n_time_events)
    {
        out << get_indent() << "struct TimeEvent" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "bool is_started {};" << std::endl;
        out << get_indent() << "bool is_periodic {};" << std::endl;
        out << get_indent() << "size_t timeout_ms {};" << std::endl;
        out << get_indent() << "size_t expire_time_ms {};" << std::endl;
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;

        out << get_indent() << "struct TimeEvents" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        for (auto i = 0u; i < n_time_events; i++)
        {
            auto ev = reader.getTimeEvent(i);
            if ((nullptr != ev) && ("null" != ev->name))
            {
                out << get_indent() << "TimeEvent " << Style::get_event_name(ev) << " {};" << std::endl;
            }
        }
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;
    }

    // create an enum of all in-event names
    if ((0 < n_in_events) || (0 < n_time_events) || (0 < n_internal_events))
    {
        out << get_indent() << "enum class EventId" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        for (auto i = 0u; i < n_in_events; i++)
        {
            auto ev = reader.getInEvent(i);
            if ((nullptr != ev) && ("null" != ev->name))
            {
                out << get_indent() << "in_" << Style::get_event_name(ev) << "," << std::endl;
            }
        }
        for (auto i = 0u; i < n_time_events; i++)
        {
            auto ev = reader.getTimeEvent(i);
            if ((nullptr != ev) && ("null" != ev->name))
            {
                out << get_indent() << "time_" << Style::get_event_name(ev) << "," << std::endl;
            }
        }
        for (auto i = 0u; i < n_internal_events; i++)
        {
            auto ev = reader.getInternalEvent(i);
            if ((nullptr != ev) && ("null" != ev->name))
            {
                out << get_indent() << "internal_" << Style::get_event_name(ev) << "," << std::endl;
            }
        }
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;

        // create a union of any event data possible.
        std::vector<std::pair<std::string, std::string>> paramData {};
        for (auto i = 0u; i < n_in_events; i++)
        {
            auto ev = reader.getInEvent(i);
            if ((nullptr != ev) && ev->require_parameter && ("null" != ev->name))
            {
                paramData.emplace_back(ev->parameter_type, "in_" + ev->name);
            }
        }
        for (std::size_t i = 0; i < n_internal_events; i++)
        {
            auto ev = reader.getInternalEvent(i);
            if ((nullptr != ev) && ev->require_parameter && ("null" != ev->name))
            {
                paramData.emplace_back(ev->parameter_type, "internal_" + ev->name);
            }
        }
        if (!paramData.empty())
        {
            out << get_indent() << "union EventData" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            for (const auto& x : paramData)
            {
                out << get_indent() << x.first << " " << x.second << " {};" << std::endl;
            }
            decrease_indent();

            out << get_indent() << "};" << std::endl << std::endl;
        }

        // create struct containing the in-event.
        out << get_indent() << "struct Event" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "EventId id {};" << std::endl;
        if (!paramData.empty())
        {
            out << get_indent() << "EventData parameter {};" << std::endl;
        }
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;
    }
}

void Writer::decl_variable_list(std::ofstream& out)
{
    const auto n_private = reader.getPrivateVariableCount();
    const auto n_public  = reader.getPublicVariableCount();

    if ((0 == n_private) && (0 == n_public))
    {
        // no variables!
    }
    else
    {
        out << get_indent() << "struct Variables" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        // write private
        if (0 < n_private)
        {
            out << get_indent() << "struct InternalVariables" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            for (auto i = 0u; i < n_private; i++)
            {
                auto var = reader.getPrivateVariable(i);
                if ((nullptr != var) && (var->is_private))
                {
                    out << get_indent() << var->type << " " << Style::get_variable_name(var) << " {};" << std::endl;
                }
            }
            decrease_indent();

            out << get_indent() << "} internal {};" << std::endl;
        }

        // write public
        if (0 < n_public)
        {
            out << get_indent() << "struct ExportedVariables" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            for (size_t i = 0; i < n_public; i++)
            {
                auto var = reader.getPublicVariable(i);
                if ((nullptr != var) && !var->is_private)
                {
                    out << get_indent() << var->type << " " << Style::get_variable_name(var) << " {};" << std::endl;
                }
            }
            decrease_indent();

            out << get_indent() << "} exported {};" << std::endl;
        }
        decrease_indent();

        out << get_indent() << "};" << std::endl << std::endl;
    }
}

void Writer::decl_tracing_callback(std::ofstream& out)
{
    if (config.do_tracing)
    {
        out << get_indent() << "using TraceEntry_t = std::function<void(" << Style::get_state_type() << " state)>;"
            << std::endl;
        out << get_indent() << "using TraceExit_t = std::function<void(" << Style::get_state_type() << " state)>;"
            << std::endl
            << std::endl;
    }
}

void Writer::decl_state_machine(std::ofstream& out)
{
    // write internal structure
    out << "///\\brief State machine base class for " << reader.get_model_name() << "." << std::endl;
    out << get_indent() << "class " << reader.get_model_name() << std::endl;
    out << get_indent() << "{" << std::endl;
    out << get_indent() << "private:" << std::endl;
    increase_indent();

    out << get_indent() << Style::get_state_type() << " state;" << std::endl;
    if (0 < reader.getTimeEventCount())
    {
        out << get_indent() << "TimeEvents time_events;" << std::endl;
    }
    if ((0 < reader.getInEventCount()) || (0 < reader.getTimeEventCount()) || (0 < reader.getInternalEventCount()))
    {
        out << get_indent() << "std::deque<Event> event_queue;" << std::endl;
    }
    if (0 < reader.getOutEventCount())
    {
        out << get_indent() << "std::deque<OutEvent> out_event_queue;" << std::endl;
    }
    if (0 < reader.get_variable_count())
    {
        out << get_indent() << "Variables variables;" << std::endl;
    }
    if (config.do_tracing)
    {
        out << get_indent() << "TraceEntry_t trace_enter_function;" << std::endl;
        out << get_indent() << "TraceExit_t trace_exit_function;" << std::endl;
    }
    if (0 < reader.getTimeEventCount())
    {
        // time now counter
        out << get_indent() << "size_t time_now_ms;" << std::endl;
    }
    out << get_indent() << "Event active_event;" << std::endl;
    out << get_indent() << "void " << Style::get_top_run_cycle() << "();" << std::endl;
    if (config.do_tracing)
    {
        out << get_indent() << "void " << Style::get_trace_entry() << "(" << Style::get_state_type() << " state);"
            << std::endl;
        out << get_indent() << "void " << Style::get_trace_exit() << "(" << Style::get_state_type() << " state);"
            << std::endl;
    }
    for (auto i = 0u; i < reader.getInternalEventCount(); i++)
    {
        auto ev = reader.getInternalEvent(i);
        if ((nullptr != ev) && ("null" != ev->name))
        {
            out << get_indent() << "void " << Style::get_event_raise(ev) << "(";
            if (ev->require_parameter)
            {
                out << ev->parameter_type << " value";
            }
            out << ");" << std::endl;
        }
    }
    for (auto i = 0u; i < reader.getOutEventCount(); i++)
    {
        auto ev = reader.getOutEvent(i);
        if ((nullptr != ev) && ("null" != ev->name))
        {
            out << get_indent() << "void " << Style::get_event_raise(ev) << "(";
            if (ev->require_parameter)
            {
                out << ev->parameter_type << " value";
            }
            out << ");" << std::endl;
        }
    }
    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if ((nullptr != state) && ("initial" != state->name) && has_entry_statement(state->id))
        {
            out << get_indent() << "void " << styler.get_state_entry(state) << "();" << std::endl;
        }
    }
    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if ((nullptr != state) && ("initial" != state->name) && has_exit_statement(state->id))
        {
            out << get_indent() << "void " << styler.get_state_exit(state) << "();" << std::endl;
        }
    }
    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if ((nullptr != state) && ("initial" == state->name) || ("final" == state->name) || state->is_choice)
        {
            // no run cycle for initial, final or choice states.
        }
        else
        {
            out << get_indent() << "bool " << styler.get_state_run_cycle(state)
                << "(const Event& event, bool try_transition);" << std::endl;
        }
    }
    out << std::endl;
    decrease_indent();

    out << get_indent() << "public:" << std::endl;
    increase_indent();

    out << get_indent() << reader.get_model_name() << "() : ";
    out << "state()";
    if (0 < reader.getTimeEventCount())
    {
        out << ", time_events()";
    }
    if ((0 < reader.getInEventCount()) || (0 < reader.getTimeEventCount()) || (0 < reader.getInternalEventCount()))
    {
        out << ", event_queue()";
    }
    if (0 < reader.getOutEventCount())
    {
        out << ", out_event_queue()";
    }
    if (0 < reader.get_variable_count())
    {
        out << ", variables()";
    }
    if (0 < reader.getTimeEventCount())
    {
        out << ", time_now_ms()";
    }
    out << " {}" << std::endl;
    out << get_indent() << "~" << reader.get_model_name() << "() = default;" << std::endl;
    // add all prototypes.
    if (config.do_tracing)
    {
        out << get_indent() << "void set_trace_enter_callback(const TraceEntry_t& enter_cb);" << std::endl;
        out << get_indent() << "void set_trace_exit_callback(const TraceExit_t& exit_cb);" << std::endl;
        out << get_indent() << "static std::string get_state_name(" << Style::get_state_type() << " s);" << std::endl;
        out << get_indent() << "[[nodiscard]] " << Style::get_state_type() << " get_state() const;" << std::endl;
    }
    out << get_indent() << "void init();" << std::endl;
    if (0 < reader.getTimeEventCount())
    {
        out << get_indent() << "void " << Style::get_time_tick() << "(" << "size_t time_elapsed_ms);" << std::endl;
    }
    for (auto i = 0u; i < reader.getInEventCount(); i++)
    {
        auto ev = reader.getInEvent(i);
        if ((nullptr != ev) && ("null" != ev->name))
        {
            out << get_indent() << "void " << Style::get_event_raise(ev) << "(";
            if (ev->require_parameter)
            {
                out << ev->parameter_type << " value";
            }
            out << ");" << std::endl;
        }
    }
    if (0 < reader.getOutEventCount())
    {
        out << get_indent() << "bool is_out_event_raised(" << reader.get_model_name() << "_OutEvent& ev);" << std::endl;
    }
    for (auto i = 0u; i < reader.get_variable_count(); i++)
    {
        auto var = reader.getPublicVariable(i);
        if (nullptr != var)
        {
            out << get_indent() << "[[nodiscard]] " << var->type << " get_" << Style::get_variable_name(var)
                << "() const;" << std::endl;
        }
    }
    decrease_indent();

    out << get_indent() << "};" << std::endl << std::endl;
}

void Writer::impl_init(std::ofstream& out, const std::vector<State*>& first_state)
{
    out << get_indent() << "void " << reader.get_model_name() << "::init()" << std::endl;
    out << get_indent() << "{" << std::endl;
    increase_indent();

    // write variable inits
    out << get_indent() << "// Initialise variables." << std::endl;
    bool any_specific_inited = false;
    for (auto i = 0u; i < reader.get_variable_count(); i++)
    {
        auto var = reader.get_variable(i);
        if (var->specific_initial_value)
        {
            if (var->is_private)
            {
                out << get_indent() << "variables.internal.";
            }
            else
            {
                out << get_indent() << "variables.exported.";
            }
            out << Style::get_variable_name(var) << " = " << var->initial_value << ";" << std::endl;
            any_specific_inited = true;
        }
    }
    if (!any_specific_inited)
    {
        out << get_indent() << "// No variables with specific values defined, all initialised to 0." << std::endl;
    }
    out << std::endl;

    // enter first state
    if (!first_state.empty())
    {
        out << get_indent() << "// Set initial state." << std::endl;
        State* targetState = nullptr;
        for (auto i : first_state)
        {
            targetState = i;
            if (has_entry_statement(targetState->id))
            {
                // write entry call
                out << get_indent() << styler.get_state_entry(targetState) << "();" << std::endl;
            }
        }
        out << get_indent() << "state = " << styler.get_state_name(targetState) << ";" << std::endl;
        if (config.do_tracing)
        {
            out << get_indent() << get_trace_call_entry(targetState) << std::endl;
        }
    }
    decrease_indent();

    out << get_indent() << "}" << std::endl << std::endl;
}

void Writer::impl_raise_in_event(std::ofstream& out)
{
    for (auto i = 0u; i < reader.getInEventCount(); i++)
    {
        auto ev = reader.getInEvent(i);
        if ((nullptr != ev) && ("null" != ev->name))
        {
            out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_event_raise(ev) << "(";
            if (ev->require_parameter)
            {
                out << ev->parameter_type << " value";
            }
            out << ")" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            out << get_indent() << "Event event {};" << std::endl;
            out << get_indent() << "event.id = " << "EventId::in_" << Style::get_event_name(ev) << ";" << std::endl;

            if (ev->require_parameter)
            {
                out << get_indent() << "event.parameter.in_" << Style::get_event_name(ev) << " = value;" << std::endl;
            }

            out << get_indent() << "event_queue.push_back(event);" << std::endl;
            out << get_indent() << Style::get_top_run_cycle() << "();" << std::endl;
            decrease_indent();

            out << get_indent() << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_raise_out_event(std::ofstream& out)
{
    for (auto i = 0u; i < reader.getOutEventCount(); i++)
    {
        auto ev = reader.getOutEvent(i);
        if ((nullptr != ev) && ("null" != ev->name))
        {
            out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_event_raise(ev) << "(";
            if (ev->require_parameter)
            {
                out << ev->parameter_type << " value";
            }
            out << ")" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            out << get_indent() << "OutEvent event {};" << std::endl;
            out << get_indent() << "event.id = OutEventId::" << Style::get_event_name(ev) << ";" << std::endl;

            if (ev->require_parameter)
            {
                out << get_indent() << "event.parameter." << Style::get_event_name(ev) << " = value;" << std::endl;
            }

            out << get_indent() << "out_event_queue.push_back(event);" << std::endl;
            decrease_indent();

            out << get_indent() << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_raise_internal_event(std::ofstream& out)
{
    for (auto i = 0u; i < reader.getInternalEventCount(); i++)
    {
        auto ev = reader.getInternalEvent(i);
        if ((nullptr != ev) && ("null" != ev->name))
        {
            out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_event_raise(ev) << "(";
            if (ev->require_parameter)
            {
                out << ev->parameter_type << " value";
            }
            out << ")" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            out << get_indent() << "Event event {};" << std::endl;
            out << get_indent() << "event.id = EventId::internal_" << Style::get_event_name(ev) << ";" << std::endl;

            if (ev->require_parameter)
            {
                out << get_indent() << "event.parameter.internal_" << Style::get_event_name(ev) << " = value;"
                    << std::endl;
            }
            out << get_indent() << "event_queue.push_back(event);" << std::endl;
            decrease_indent();

            out << get_indent() << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_check_out_event(std::ofstream& out)
{
    if (0 < reader.getOutEventCount())
    {
        out << get_indent() << "bool " << reader.get_model_name() << "::is_out_event_raised(" << reader.get_model_name()
            << "_OutEvent& ev)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "bool pending = false;" << std::endl;
        out << get_indent() << "if (!out_event_queue.empty())" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "ev = out_event_queue.front();" << std::endl;
        out << get_indent() << "out_event_queue.pop_front();" << std::endl;
        out << get_indent() << "pending = true;" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl;
        out << get_indent() << "return pending;" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;
    }
}

void Writer::impl_get_variable(std::ofstream& out)
{
    for (auto i = 0u; i < reader.get_variable_count(); i++)
    {
        auto var = reader.getPublicVariable(i);
        if (nullptr != var)
        {
            out << get_indent() << var->type << " " << reader.get_model_name() << "::get_"
                << Style::get_variable_name(var) << "() const" << std::endl;
            out << "{" << std::endl;
            increase_indent();

            out << get_indent() << "return variables.exported." << Style::get_variable_name(var) << ";" << std::endl;
            decrease_indent();

            out << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_time_tick(std::ofstream& out)
{
    if (0 < reader.getTimeEventCount())
    {
        out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_time_tick()
            << "(size_t time_elapsed_ms)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "time_now_ms += time_elapsed_ms;" << std::endl << std::endl;
        for (auto i = 0u; i < reader.getTimeEventCount(); i++)
        {
            auto ev = reader.getTimeEvent(i);
            if (nullptr != ev)
            {
                out << get_indent() << "if (time_events." << Style::get_event_name(ev) << ".is_started)" << std::endl;
                out << get_indent() << "{" << std::endl;
                increase_indent();

                out << get_indent() << "if (time_events." << Style::get_event_name(ev)
                    << ".expire_time_ms <= time_now_ms)" << std::endl;
                out << get_indent() << "{" << std::endl;
                increase_indent();

                out << get_indent() << "// Time events does not carry any parameter." << std::endl;
                out << get_indent() << "Event event {};" << std::endl;
                out << get_indent() << "event.id = " << "EventId::time_" << Style::get_event_name(ev) << ";"
                    << std::endl;
                out << get_indent() << "event_queue.push_back(event);" << std::endl << std::endl;

                out << get_indent() << "// Check for automatic reload." << std::endl;
                out << get_indent() << "if (time_events." << Style::get_event_name(ev) << ".is_periodic)" << std::endl;
                out << get_indent() << "{" << std::endl;
                increase_indent();

                out << get_indent() << "time_events." << Style::get_event_name(ev) << ".expire_time_ms += time_events."
                    << Style::get_event_name(ev) << ".timeout_ms;" << std::endl;
                out << get_indent() << "time_events." << Style::get_event_name(ev) << ".is_started = true;"
                    << std::endl;
                decrease_indent();

                out << get_indent() << "}" << std::endl;
                out << get_indent() << "else" << std::endl;
                out << get_indent() << "{" << std::endl;
                increase_indent();

                out << get_indent() << "time_events." << Style::get_event_name(ev) << ".is_started = false;"
                    << std::endl;
                decrease_indent();

                out << get_indent() << "}" << std::endl;
                decrease_indent();

                out << get_indent() << "}" << std::endl;
                decrease_indent();

                out << get_indent() << "}" << std::endl;
            }
        }
        out << get_indent() << Style::get_top_run_cycle() << "();" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;
    }
}

void Writer::impl_top_run_cycle(std::ofstream& out)
{
    size_t writeNumber = 0;
    out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_top_run_cycle() << "()"
        << std::endl;
    out << get_indent() << "{" << std::endl;
    increase_indent();

    out << get_indent() << "// Handle all queued events." << std::endl;
    out << get_indent() << "while (!event_queue.empty())" << std::endl;
    out << get_indent() << "{" << std::endl;
    increase_indent();

    out << get_indent() << "active_event = event_queue.front();" << std::endl;
    out << get_indent() << "event_queue.pop_front();" << std::endl << std::endl;
    out << get_indent() << "switch (state)" << std::endl;
    out << get_indent() << "{" << std::endl;
    increase_indent();

    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if (("initial" == state->name) || ("final" == state->name) || state->is_choice)
        {
            // no handling on initial or final states, or choice.
        }
        else
        {
            out << get_indent() << "case " << styler.get_state_name(state) << ":" << std::endl;
            increase_indent();

            out << get_indent() << styler.get_state_run_cycle(state) << "(active_event, true);" << std::endl;
            out << get_indent() << "break;" << std::endl << std::endl;
            decrease_indent();
        }
    }
    if (0 < reader.getStateCount())
    {
        out << get_indent() << "default:" << std::endl;
        increase_indent();

        out << get_indent() << "// Invalid, or final state." << std::endl;
        out << get_indent() << "break;" << std::endl;
        decrease_indent();
    }
    decrease_indent();

    out << get_indent() << "}" << std::endl;
    decrease_indent();

    out << get_indent() << "}" << std::endl;
    decrease_indent();

    out << get_indent() << "}" << std::endl << std::endl;
}

void Writer::impl_trace_calls(std::ofstream& out)
{
    if (config.do_tracing)
    {
        out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_trace_entry() << "("
            << Style::get_state_type() << " entered_state)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "if (nullptr != trace_enter_function)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "trace_enter_function(entered_state);" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;

        out << get_indent() << "void " << reader.get_model_name() << "::" << Style::get_trace_exit() << "("
            << Style::get_state_type() << " exited_state)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "if (nullptr != trace_exit_function)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "trace_exit_function(exited_state);" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;

        out << get_indent() << "void " << reader.get_model_name()
            << "::set_trace_enter_callback(const TraceEntry_t& enter_cb)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "trace_enter_function = enter_cb;" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;

        out << get_indent() << "void " << reader.get_model_name()
            << "::set_trace_exit_callback(const TraceExit_t& exit_cb)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "trace_exit_function = exit_cb;" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;

        out << get_indent() << "std::string " << reader.get_model_name() << "::get_state_name("
            << Style::get_state_type() << " s)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "switch (s)" << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        for (auto i = 0u; i < reader.getStateCount(); i++)
        {
            auto s = reader.getState(i);
            /* Only write down state on actual states that the machine may stay in. */
            if ((nullptr != s) && ("initial" != s->name) && ("final" != s->name) && (!s->is_choice))
            {
                out << get_indent() << "case " << Style::get_state_type() << "::" << styler.get_state_name_pure(s)
                    << ":" << std::endl;
                increase_indent();

                out << get_indent() << "return \"" << styler.get_state_name_pure(s) << "\";" << std::endl << std::endl;
                decrease_indent();
            }
        }

        out << get_indent() << "default:" << std::endl;
        increase_indent();

        out << get_indent() << "// Invalid state." << std::endl;
        out << get_indent() << "return {};" << std::endl;
        decrease_indent();
        decrease_indent();

        out << get_indent() << "}" << std::endl;
        decrease_indent();

        out << get_indent() << "}" << std::endl << std::endl;

        out << get_indent() << Style::get_state_type() << " " << reader.get_model_name() << "::get_state() const"
            << std::endl;
        out << get_indent() << "{" << std::endl;
        increase_indent();

        out << get_indent() << "return state;" << std::endl;
        decrease_indent();

        out << "}" << std::endl << std::endl;
    }
}

void Writer::impl_run_cycle(std::ofstream& out)
{
    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if (("initial" == state->name) || ("final" == state->name) || state->is_choice)
        {
            // initial, final or choice state, no runcycle
        }
        else
        {
            bool isEmptyBody = true;
            auto startIndent = indent;

            out << get_indent() << "bool " << reader.get_model_name() << "::" << styler.get_state_run_cycle(state)
                << "(const Event& event, bool try_transition)" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            // write comment declaration here if one exists.
            const auto numCommentLines = reader.getDeclCount(state->id, Declaration::Comment);
            if (0 < numCommentLines)
            {
                isEmptyBody = false;
                for (auto j = 0u; j < numCommentLines; j++)
                {
                    auto decl = reader.getDeclFromStateId(state->id, Declaration::Comment, j);
                    out << get_indent() << "// " << decl->declaration << std::endl;
                }
                out << std::endl;
            }

            out << get_indent() << "auto did_transition = try_transition;" << std::endl;
            out << get_indent() << "if (try_transition)" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            const size_t nOutTr = reader.getTransitionCountFromStateId(state->id);

            // write parent react
            auto parentState = reader.getStateById(state->parent);
            if (nullptr != parentState)
            {
                isEmptyBody = false;
                {
                    out << get_indent() << "if (!" << styler.get_state_run_cycle(parentState)
                        << "(event, try_transition))" << std::endl;
                    out << get_indent() << "{" << std::endl;
                    increase_indent();
                }
            }

            if (0 == nOutTr)
            {
                out << get_indent() << "did_transition = false;" << std::endl;
            }
            else
            {
                for (auto j = 0u; j < nOutTr; j++)
                {
                    auto tr = reader.getTransitionFrom(state->id, j);
                    if ("null" == tr->event.name)
                    {
                        auto trStB = reader.getStateById(tr->state_b);
                        if (nullptr == trStB)
                        {
                            error_report("Null transition!", __LINE__);
                        }
                        else if ("final" != trStB->name)
                        {
                            error_report("Null transition!", __LINE__);

                            // handle as a oncycle transition?
                            isEmptyBody = false;

                            out << get_indent() << get_if_else_if(j) << " (true)" << std::endl;
                            out << get_indent() << "{" << std::endl;
                            increase_indent();

                            // is exit function exists
                            if (has_exit_statement(state->id))
                            {
                                out << get_indent() << styler.get_state_exit(state) << "();" << std::endl;
                            }

                            decrease_indent();
                            out << get_indent() << "}" << std::endl;
                        }
                    }
                    else
                    {
                        auto trStB = reader.getStateById(tr->state_b);
                        if (nullptr == trStB)
                        {
                            error_report("Null transition!", __LINE__);
                        }
                        else
                        {
                            isEmptyBody = false;

                            if (tr->event.is_time_event)
                            {
                                if (tr->has_guard)
                                {
                                    std::string guardStr = parse_guard(tr->guard);
                                    out << get_indent() << get_if_else_if(j) << " (("
                                        << "EventId::time_" << Style::get_event_name(&tr->event) << " == event.id) && ("
                                        << guardStr << "))" << std::endl;
                                }
                                else
                                {
                                    out << get_indent() << get_if_else_if(j) << " ("
                                        << "EventId::time_" << Style::get_event_name(&tr->event) << " == event.id)"
                                        << std::endl;
                                }
                            }
                            else
                            {
                                if (tr->has_guard)
                                {
                                    std::string guardStr = parse_guard(tr->guard);
                                    out << get_indent() << get_if_else_if(j);
                                    if (EventDirection::Incoming == tr->event.direction)
                                    {
                                        out << " ((EventId::in_" << Style::get_event_name(&tr->event)
                                            << " == event.id) && (";
                                    }
                                    else if (EventDirection::Internal == tr->event.direction)
                                    {
                                        out << " ((EventId::internal_" << Style::get_event_name(&tr->event)
                                            << " == event.id) && (";
                                    }
                                    else
                                    {
                                        out << " ((EventId::out_" << Style::get_event_name(&tr->event)
                                            << " == event.id) && (";
                                    }
                                    out << guardStr << "))" << std::endl;
                                }
                                else
                                {
                                    if (EventDirection::Incoming == tr->event.direction)
                                    {
                                        out << get_indent() << get_if_else_if(j) << " (EventId::in_"
                                            << Style::get_event_name(&tr->event) << " == event.id)" << std::endl;
                                    }
                                    else if (EventDirection::Internal == tr->event.direction)
                                    {
                                        out << get_indent() << get_if_else_if(j) << " (EventId::internal_"
                                            << Style::get_event_name(&tr->event) << " == event.id)" << std::endl;
                                    }
                                    else
                                    {
                                        out << get_indent() << get_if_else_if(j) << " (EventId::out_"
                                            << Style::get_event_name(&tr->event) << " == event.id)" << std::endl;
                                    }
                                }
                            }
                            out << get_indent() << "{" << std::endl;
                            increase_indent();

                            const bool didChildExits = parse_child_exits(out, state, state->id, false);

                            if (didChildExits)
                            {
                                out << std::endl;
                            }
                            else
                            {
                                if (has_exit_statement(state->id))
                                {
                                    out << get_indent() << "// Handle super-step exit." << std::endl;
                                    out << get_indent() << styler.get_state_exit(state) << "();" << std::endl;
                                }
                                if (config.do_tracing)
                                {
                                    out << get_indent() << get_trace_call_exit(state) << std::endl;
                                }
                                /* Extra new-line */
                                if ((has_exit_statement(state->id)) || (config.do_tracing))
                                {
                                    out << std::endl;
                                }
                            }

                            // TODO: do entry actins on all states entered
                            // towards the goal! Might needs some work..
                            auto enteredStates = find_entry_state(trStB);

                            if (!enteredStates.empty())
                            {
                                out << get_indent() << "// Handle super-step entry." << std::endl;
                            }

                            State* finalState = nullptr;
                            for (auto& enteredState : enteredStates)
                            {
                                finalState = enteredState;

                                if (has_entry_statement(finalState->id))
                                {
                                    out << get_indent() << styler.get_state_entry(finalState) << "();" << std::endl;
                                }

                                if (config.do_tracing)
                                {
                                    // Don't trace entering the choice states, since the state does not exist.
                                    if (!finalState->is_choice)
                                    {
                                        out << get_indent() << get_trace_call_entry(finalState) << std::endl;
                                    }
                                }
                            }

                            // handle choice node?
                            if ((nullptr != finalState) && (finalState->is_choice))
                            {
                                parse_choice_path(out, finalState);
                            }
                            else
                            {
                                out << get_indent() << "state = " << styler.get_state_name(finalState) << ";"
                                    << std::endl;
                            }
                            decrease_indent();

                            out << get_indent() << "}" << std::endl;
                        }
                    }
                }

                out << get_indent() << "else" << std::endl;
                out << get_indent() << "{" << std::endl;
                increase_indent();

                out << get_indent() << "did_transition = false;" << std::endl;
                decrease_indent();

                out << get_indent() << "}" << std::endl;
            }

            while (startIndent + 1 < indent)
            {
                decrease_indent();
                out << get_indent() << "}" << std::endl;
            }

            out << get_indent() << "return did_transition;" << std::endl;
            decrease_indent();

            out << get_indent() << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_entry_action(std::ofstream& out)
{
    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if ("initial" != state->name)
        {
            const auto numDecl   = reader.getDeclCount(state->id, Declaration::Entry);
            size_t     numTimeEv = 0;
            for (auto j = 0u; j < reader.getTransitionCountFromStateId(state->id); j++)
            {
                auto tr = reader.getTransitionFrom(state->id, j);
                if ((nullptr != tr) && (tr->event.is_time_event))
                {
                    numTimeEv++;
                }
            }

            if ((0 < numDecl) || (0 < numTimeEv))
            {
                out << get_indent() << "void " << reader.get_model_name() << "::" << styler.get_state_entry(state)
                    << "()" << std::endl;
                out << get_indent() << "{" << std::endl;

                // start timers
                size_t writeIndex = 0;
                increase_indent();

                for (auto j = 0u; j < reader.getTransitionCountFromStateId(state->id); j++)
                {
                    auto tr = reader.getTransitionFrom(state->id, j);
                    if ((nullptr != tr) && (tr->event.is_time_event))
                    {
                        out << get_indent() << "/* Start timer " << Style::get_event_name(&tr->event)
                            << " with timeout of " << tr->event.expire_time_ms << " ms. */" << std::endl;
                        out << get_indent() << "time_events." << Style::get_event_name(&tr->event)
                            << ".timeout_ms = " << tr->event.expire_time_ms << ";" << std::endl;
                        out << get_indent() << "time_events." << Style::get_event_name(&tr->event)
                            << ".expire_time_ms = time_now_ms + " << tr->event.expire_time_ms << ";" << std::endl;
                        out << get_indent() << "time_events." << Style::get_event_name(&tr->event)
                            << ".is_periodic = " << (tr->event.is_periodic ? "true;" : "false;") << std::endl;
                        out << get_indent() << "time_events." << Style::get_event_name(&tr->event)
                            << ".is_started = true;" << std::endl;
                        writeIndex++;
                        if (writeIndex < numTimeEv)
                        {
                            out << std::endl;
                        }
                    }
                }

                if ((0 < numDecl) && (0 < numTimeEv))
                {
                    // add a space between the parts
                    out << std::endl;
                }

                for (auto j = 0u; j < numDecl; j++)
                {
                    auto decl = reader.getDeclFromStateId(state->id, Declaration::Entry, j);
                    if (Declaration::Entry == decl->type)
                    {
                        parse_declaration(out, decl->declaration);
                    }
                }
                decrease_indent();

                out << get_indent() << "}" << std::endl << std::endl;
            }
        }
    }
}

void Writer::impl_exit_action(std::ofstream& out)
{
    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if ("initial" != state->name)
        {
            const auto numDecl   = reader.getDeclCount(state->id, Declaration::Exit);
            size_t     numTimeEv = 0;
            for (auto j = 0u; j < reader.getTransitionCountFromStateId(state->id); j++)
            {
                auto tr = reader.getTransitionFrom(state->id, j);
                if ((nullptr != tr) && (tr->event.is_time_event))
                {
                    numTimeEv++;
                }
            }

            if ((0 < numDecl) || (0 < numTimeEv))
            {
                out << get_indent() << "void " << reader.get_model_name() << "::" << styler.get_state_exit(state)
                    << "()" << std::endl;
                out << get_indent() << "{" << std::endl;

                // stop timers
                increase_indent();
                for (auto j = 0u; j < reader.getTransitionCountFromStateId(state->id); j++)
                {
                    auto tr = reader.getTransitionFrom(state->id, j);
                    if ((nullptr != tr) && (tr->event.is_time_event))
                    {
                        out << get_indent() << "time_events." << Style::get_event_name(&tr->event)
                            << ".is_started = false;" << std::endl;
                    }
                }

                if ((0 < numDecl) && (0 < numTimeEv))
                {
                    // add a space between the parts
                    out << std::endl;
                }

                if (0 < numDecl)
                {
                    for (auto j = 0u; j < numDecl; j++)
                    {
                        auto decl = reader.getDeclFromStateId(state->id, Declaration::Exit, j);
                        if (Declaration::Exit == decl->type)
                        {
                            parse_declaration(out, decl->declaration);
                        }
                    }
                }
                decrease_indent();

                out << get_indent() << "}" << std::endl << std::endl;
            }
        }
    }
}

std::vector<std::string> Writer::tokenize(const std::string& str)
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

void Writer::parse_declaration(std::ofstream& out, const std::string& declaration)
{
    // replace all X that corresponds with an event name with handle->events.X.param
    // also replace any word found that corresponds to a variable to its
    // correct location, e.g., bus -> handle->variables.private.bus
    std::string wstr {};

    auto   isParsed = false;
    size_t start    = 0;

    while (!isParsed)
    {
        const size_t replaceStart = declaration.find_first_of('$', start);
        if (std::string::npos == replaceStart)
        {
            // copy remains
            wstr += declaration.substr(start);
            isParsed = true;
        }
        else
        {
            // copy until start of replacement
            const size_t firstLength = (replaceStart - start);
            wstr += declaration.substr(start, firstLength);
            const size_t replaceEnd = declaration.find_first_of('}', replaceStart);
            if (std::string::npos == replaceEnd)
            {
                // error!
                std::cout << "Error: Invalid format of variable/event." << std::endl;
                isParsed = true;
            }
            else
            {
                const auto        replaceLength = (replaceEnd - replaceStart) - 2;
                const std::string replaceString = declaration.substr(replaceStart + 2, replaceLength);
                bool              isReplaced    = false;

                for (auto i = 0u; i < reader.get_variable_count(); i++)
                {
                    auto var = reader.get_variable(i);
                    if (replaceString == var->name)
                    {
                        wstr += "variables.";
                        if (var->is_private)
                        {
                            wstr += "internal.";
                        }
                        else
                        {
                            wstr += "exported.";
                        }
                        wstr += Style::get_variable_name(var);
                        isReplaced = true;
                        break;
                    }
                }

                if (!isReplaced)
                {
                    for (auto i = 0u; i < reader.getInEventCount(); i++)
                    {
                        auto ev = reader.getInEvent(i);
                        if (replaceString == ev->name)
                        {
                            switch (ev->direction)
                            {
                                case EventDirection::Incoming:
                                    wstr += "active_event.parameter.in_";
                                    break;

                                case EventDirection::Outgoing:
                                    wstr += "active_event.parameter.out_";
                                    break;

                                case EventDirection::Internal:
                                    wstr += "active_event.parameter.internal_";
                                    break;

                                default:
                                    break;
                            }
                            wstr += Style::get_event_name(ev);
                            isReplaced = true;
                            break;
                        }
                    }
                }

                if (!isReplaced)
                {
                    wstr += "/* TODO */";
                }

                start = replaceEnd + 1;
            }
        }
    }

    // search for "raise X"
    std::string outstr {};
    isParsed = false;
    start    = 0;

    while (!isParsed)
    {
        const size_t raiseKeywordPosition = wstr.find("raise", start);
        if (std::string::npos == raiseKeywordPosition)
        {
            // finished without finding 'raise'
            outstr += wstr.substr(start);
            isParsed = true;
        }
        else
        {
            // find next space
            const size_t firstSpacePosition = wstr.find_first_of(' ', raiseKeywordPosition);
            if (std::string::npos == firstSpacePosition)
            {
                // no space detected after the keyword
                outstr += wstr.substr(start);
                isParsed = true;
            }
            else
            {
                // tokenize the string.
                auto tokens = tokenize(wstr.substr(firstSpacePosition + 1));
                if (!tokens.empty())
                {
                    auto ev = reader.findEvent(tokens[0]);
                    if (nullptr == ev)
                    {
                        outstr += "/* Trying to raise undeclared event '" + tokens[0] + "' */";
                    }
                    else
                    {
                        outstr += Style::get_event_raise(tokens[0]) + "(";
                        if (ev->require_parameter)
                        {
                            if (tokens.size() < 2)
                            {
                                outstr += "{}";
                            }
                            else
                            {
                                outstr += tokens[1];
                            }
                        }
                    }
                    outstr += ");";
                    isParsed = true;
                }
            }
        }
    }

    if (';' != outstr.back())
    {
        outstr += ";";
    }

    out << get_indent() << outstr << std::endl;
}

std::string Writer::parse_guard(const std::string& guardStrRaw)
{
    // replace all X that corresponds with an event name with handle->events.X.param
    // also replace any word found that corresponds to a variable to its
    // correct location, e.g., bus -> handle->variables.private.bus
    std::string wstr {};

    auto   isParsed = false;
    size_t start    = 0;

    while (!isParsed)
    {
        const size_t replaceStart = guardStrRaw.find_first_of('$', start);
        if (std::string::npos == replaceStart)
        {
            // copy remains
            wstr += guardStrRaw.substr(start);
            isParsed = true;
        }
        else
        {
            // copy until start of replacement
            const size_t firstLength = (replaceStart - start);
            wstr += guardStrRaw.substr(start, firstLength);
            const size_t replaceEnd = guardStrRaw.find_first_of('}', replaceStart);
            if (std::string::npos == replaceEnd)
            {
                // error!
                std::cout << "Error: Invalid format of variable/event." << std::endl;
                isParsed = true;
            }
            else
            {
                const size_t      replaceLength = (replaceEnd - replaceStart) - 2;
                const std::string replaceString = guardStrRaw.substr(replaceStart + 2, replaceLength);
                bool              isReplaced    = false;

                for (auto i = 0u; i < reader.get_variable_count(); i++)
                {
                    auto var = reader.get_variable(i);
                    if (replaceString == var->name)
                    {
                        wstr += "variables.";
                        if (var->is_private)
                        {
                            wstr += "internal.";
                        }
                        else
                        {
                            wstr += "exported.";
                        }
                        wstr += var->name;
                        isReplaced = true;
                        break;
                    }
                }

                if (!isReplaced)
                {
                    for (auto i = 0u; i < reader.getInEventCount(); i++)
                    {
                        auto ev = reader.getInEvent(i);
                        if (replaceString == ev->name)
                        {
                            wstr += "events.inEvents." + ev->name + ".param";
                            isReplaced = true;
                            break;
                        }
                    }
                }

                if (!isReplaced)
                {
                    wstr += "/* TODO */";
                }

                start = replaceEnd + 1;
            }
        }
    }

    return (wstr);
}

void Writer::parse_choice_path(std::ofstream& out, State* state)
{
    // check all transitions from the choice..
    out << std::endl << get_indent() << "/* Choice: " << state->name << " */" << std::endl;

    const size_t numChoiceTr = reader.getTransitionCountFromStateId(state->id);
    if (numChoiceTr < 2)
    {
        error_report("Ony one transition from choice " + state->name, __LINE__);
    }
    else
    {
        Transition* defaultTr = nullptr;
        size_t      k         = 0;

        for (auto j = 0u; j < numChoiceTr; j++)
        {
            auto tr = reader.getTransitionFrom(state->id, j);
            if (!tr->has_guard)
            {
                defaultTr = tr;
            }
            else
            {
                // handle if statement
                out << get_indent() << get_if_else_if(k++) << " (" << parse_guard(tr->guard) << ")" << std::endl;
                out << get_indent() << "{" << std::endl;
                increase_indent();

                auto guardedState = reader.getStateById(tr->state_b);
                out << get_indent() << "// goto: " << guardedState->name << std::endl;

                if (nullptr == guardedState)
                {
                    error_report("Invalid transition from choice " + state->name, __LINE__);
                }
                else
                {
                    auto   enteredStates = find_entry_state(guardedState);
                    State* finalState    = nullptr;

                    for (auto& enteredState : enteredStates)
                    {
                        finalState = enteredState;

                        // TODO: fix call so send state name.
#if 0
                        if (writerConfig.doTracing)
                        {
                            out_c << get_indent(indentLevel + 1) << getTraceCall_entry(reader) << std::endl;
                        }
#endif

                        if (0 < reader.getDeclCount(finalState->id, Declaration::Entry))
                        {
                            out << get_indent() << styler.get_state_entry(finalState) << "();" << std::endl;
                        }
                    }
                    if (nullptr != finalState)
                    {
                        if (finalState->is_choice)
                        {
                            // nest ..
                            parse_choice_path(out, finalState);
                        }
                        else
                        {
                            out << get_indent() << "state = " << styler.get_state_name(finalState) << ";" << std::endl;
                        }
                    }
                }
                decrease_indent();

                out << get_indent() << "}" << std::endl;
            }
        }
        if (nullptr != defaultTr)
        {
            // write default transition.
            out << get_indent() << "else" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            auto guardedState = reader.getStateById(defaultTr->state_b);
            out << get_indent() << "// goto: " << guardedState->name << std::endl;

            if (nullptr == guardedState)
            {
                error_report("Invalid transition from choice " + state->name, __LINE__);
            }
            else
            {
                auto   enteredStates = find_entry_state(guardedState);
                State* finalState    = nullptr;
                for (auto& enteredState : enteredStates)
                {
                    finalState = enteredState;

                    // TODO: fix call so send state name.
#if 0
                    if (writerConfig.doTracing)
                    {
                        writer->out_c << get_indent(indentLevel + 1) << getTraceCall_entry(reader) << std::endl;
                    }
#endif

                    if (0 < reader.getDeclCount(finalState->id, Declaration::Entry))
                    {
                        out << get_indent() << styler.get_state_entry(finalState) << "();" << std::endl;
                    }
                }
                if (nullptr != finalState)
                {
                    if (finalState->is_choice)
                    {
                        // nest ..
                        parse_choice_path(out, finalState);
                    }
                    else
                    {
                        out << get_indent() << "state = " << styler.get_state_name(finalState) << ";" << std::endl;
                    }
                }
            }
            decrease_indent();

            out << get_indent() << "}" << std::endl;
        }
        else
        {
            error_report("No default transition from " + state->name, __LINE__);
        }
    }
}

std::vector<State*> Writer::get_child_states(State* currentState)
{
    std::vector<State*> childStates;
    for (auto j = 0u; j < reader.getStateCount(); j++)
    {
        auto child = reader.getState(j);

        if ((nullptr != child) && (child->parent == currentState->id) && ("initial" != child->name)
            && (0 != child->name.compare("final")) && (!child->is_choice))
        {
            childStates.push_back(child);
        }
    }
    return (childStates);
}

bool Writer::parse_child_exits(std::ofstream& out, State* currentState, StateId topState, bool didPreviousWrite)
{
    bool didWrite = didPreviousWrite;

    std::vector<State*> children  = get_child_states(currentState);
    const size_t        nChildren = children.size();

    if (0 == nChildren)
    {
        // need to detect here if any state from current to top state has exit
        // actions..
        auto tmpState      = currentState;
        bool hasExitAction = false;
        while (topState != tmpState->id)
        {
            if (has_exit_statement(tmpState->id))
            {
                hasExitAction = true;
                break;
            }
            tmpState = reader.getStateById(tmpState->parent);
        }

        if (hasExitAction)
        {
            if (!didWrite)
            {
                out << get_indent() << "/* Handle super-step exit. */" << std::endl;
            }
            out << get_indent() << get_if_else_if(didWrite ? 1 : 0) << " (" << styler.get_state_name(currentState)
                << " == state)" << std::endl;
            out << get_indent() << "{" << std::endl;
            increase_indent();

            if (has_exit_statement(currentState->id))
            {
                out << get_indent() << styler.get_state_exit(currentState) << "();" << std::endl;
            }

            if (config.do_tracing)
            {
                out << get_indent() << get_trace_call_exit(currentState) << std::endl;
            }

            // go up to the top
            while (topState != currentState->id)
            {
                currentState = reader.getStateById(currentState->parent);
                if (has_exit_statement(currentState->id))
                {
                    out << get_indent() << styler.get_state_exit(currentState) << "();" << std::endl;
                }
                if (config.do_tracing)
                {
                    out << get_indent() << get_trace_call_exit(currentState) << std::endl;
                }
            }
            decrease_indent();

            out << get_indent() << "}" << std::endl;
            didWrite = true;
        }
    }
    else
    {
        for (auto j = 0u; j < nChildren; j++)
        {
            auto child = children[j];
            didWrite   = parse_child_exits(out, child, topState, didWrite);
        }
    }

    return (didWrite);
}

bool Writer::has_entry_statement(StateId stateId)
{
    if (0u < reader.getDeclCount(stateId, Declaration::Entry))
    {
        return true;
    }
    else
    {
        for (auto j = 0u; j < reader.getTransitionCountFromStateId(stateId); j++)
        {
            if (reader.getTransitionFrom(stateId, j)->event.is_time_event)
            {
                return (true);
            }
        }
    }
    return (false);
}

bool Writer::has_exit_statement(StateId stateId)
{
    if (0u < reader.getDeclCount(stateId, Declaration::Exit))
    {
        return (true);
    }
    else
    {
        for (auto j = 0u; j < reader.getTransitionCountFromStateId(stateId); j++)
        {
            if (reader.getTransitionFrom(stateId, j)->event.is_time_event)
            {
                return (true);
            }
        }
    }
    return (false);
}

std::string Writer::get_trace_call_entry(const State* state)
{
    return Style::get_trace_entry() + "(" + styler.get_state_name(state) + ");";
}

std::string Writer::get_trace_call_exit(const State* state)
{
    return Style::get_trace_exit() + "(" + styler.get_state_name(state) + ");";
}

std::vector<State*> Writer::find_entry_state(State* in)
{
    // this function will check if the in state contains an initial sub-state,
    // and follow the path until a state is reached that does not contain an
    // initial sub-state.
    std::vector<State*> states;
    states.push_back(in);

    // check if the state contains any initial sub-state
    bool foundNext = true;
    while (foundNext)
    {
        foundNext = false;
        for (auto i = 0u; i < reader.getStateCount(); i++)
        {
            auto tmp = reader.getState(i);
            if ((in->id != tmp->id) && (in->id == tmp->parent) && ("initial" == tmp->name))
            {
                // a child state was found that is an initial state. Get transition
                // from this initial state, it should be one and only one.
                auto tr = reader.getTransitionFrom(tmp->id, 0);
                if (nullptr == tr)
                {
                    error_report("Initial state in [" + styler.get_state_name(in) + "] as no transitions.", __LINE__);
                }
                else
                {
                    // get the transition
                    tmp = reader.getStateById(tr->state_b);
                    if (nullptr == tmp)
                    {
                        error_report("Initial state in [" + styler.get_state_name(in) + "] has no target.", __LINE__);
                    }
                    else
                    {
                        // recursively check the state, we can do this by
                        // exiting the for loop but continuing from the top.
                        states.push_back(tmp);
                        in = tmp;

                        // if the state is a choice, we need to stop here.
                        foundNext = !tmp->is_choice;
                    }
                }
            }
        }
    }

    return (states);
}

std::vector<State*> Writer::find_final_state(State* in)
{
    // this function will check if the in state has an outgoing transition to
    // a final state and follow the path until a state is reached that does not
    // contain an initial sub-state.
    std::vector<State*> states;
    states.push_back(in);

    // find parent to this state
    bool foundNext = true;
    while (foundNext)
    {
        foundNext = false;
        for (auto i = 0u; i < reader.getStateCount(); i++)
        {
            auto tmp = reader.getState(i);
            if ((in->id != tmp->id) && (in->parent == tmp->id) && ("initial" != tmp->name))
            {
                // a parent state was found that is not an initial state. Check for
                // any outgoing transitions from this state to a final state. Store
                // the id of the tmp state, since we will reuse this pointer.
                const auto tmpId = tmp->id;
                for (auto j = 0u; j < reader.getTransitionCountFromStateId(tmpId); j++)
                {
                    auto tr = reader.getTransitionFrom(tmp->id, j);
                    tmp     = reader.getStateById(tr->state_b);
                    if ((nullptr != tmp) && ("final" == tmp->name))
                    {
                        // recursively check the state
                        states.push_back(tmp);
                        in = tmp;

                        // if this state is a choice, we need to stop there.
                        foundNext = !tmp->is_choice;
                    }
                }
            }
        }
    }

    return (states);
}

std::vector<State*> Writer::find_init_state()
{
    std::vector<State*> states;

    for (auto i = 0u; i < reader.getStateCount(); i++)
    {
        auto state = reader.getState(i);
        if ("initial" == state->name)
        {
            // check if this is top initial state
            if (0 == state->parent)
            {
                // this is the top initial, find transition from this idle
                auto tr = reader.getTransitionFrom(state->id, 0);
                if (nullptr == tr)
                {
                    error_report("No transition from initial state", __LINE__);
                }
                else
                {
                    // check target state (from top)
                    auto trStB = reader.getStateById(tr->state_b);
                    if (nullptr == trStB)
                    {
                        error_report("Transition to null state", __LINE__);
                    }
                    else
                    {
                        // get state where it stops.
                        states = find_entry_state(trStB);
                    }
                }
            }
        }
    }

    return (states);
}

std::string Writer::get_indent() const
{
    std::string s {};
    for (size_t i = 0; i < indent; i++)
    {
        s += "    ";
    }
    return (s);
}

std::string Writer::get_if_else_if(const size_t i)
{
    std::string s = "if";
    if (0 != i)
    {
        s = "else if";
    }
    return (s);
}
