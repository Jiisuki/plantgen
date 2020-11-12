/** @file
 *  @brief Implementation of the writer class.
 */

#include "writer.h"

#include <common.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>

void Writer::generate(const std::string &filename, const std::string &outdir)
{
    if ((nullptr == collection) || (nullptr == styler))
    {
        errorReport("Invalid use of Writer", __LINE__);
        return;
    }

    const std::string outfile_c = outdir + collection->get_model_name() + ".c";
    const std::string outfile_h = outdir + collection->get_model_name() + ".h";

    if (verbose)
    {
        std::cout << "Generating code from '" << filename << "' > '" << outfile_c << "' and '" << outfile_h << "' ..." << std::endl;
    }

    out_c.open(outfile_c);
    if (!out_c.is_open())
    {
        this->errorReport("Failed to open output file, does directory exist?", __LINE__);
        return;
    }

    out_h.open(outfile_h);
    if (!out_h.is_open())
    {
        this->errorReport("Failed to open output file, does directory exist?", __LINE__);
        return;

    }

    out_h << "/** @file" << std::endl;
    out_h << " *  @brief Interface to the " << collection->get_model_name() << " state machine." << std::endl;
    out_h << " *" << std::endl;
    out_h << " *  @startuml" << std::endl;
    collection->stream_uml_lines(out_h, " *  ");
    out_h << " *  @enduml" << std::endl;
    out_h << " */" << std::endl << std::endl;

    out_h << getIndent(0) << "#include <stdint.h>" << std::endl;
    out_h << getIndent(0) << "#include <stdbool.h>" << std::endl;
    out_h << getIndent(0) << "#include <stddef.h>" << std::endl;
    collection->stream_imports(out_h, "");
    out_h << std::endl;

    // write statechart specific types.
    decl_types();

    // write all states to .h
    decl_stateList();

    // write all event types
    decl_eventList();

    // write all variables
    decl_variableList();

    // write main declaration
    decl_stateMachine();

    // write required implementation
    if (enable_tracing)
    {
        out_h << "/* The state machine uses tracing, therefore the following functions" << std::endl;
        out_h << " * are required to be implemented by the user. */" << std::endl;
        proto_traceEntry();
        proto_traceExit();
        out_h << std::endl;
    }

    if (!styler->get_config_use_builtin_time_event_handling())
    {
        out_h << "/* The state machine uses external time event handling, therefore the" << std::endl;
        out_h << " * following functions are required to be implemented by the user. */" << std::endl;
        proto_startTimer();
        proto_stopTimer();
        out_h << std::endl;
    }

    // write init function
    out_h << "/** @brief Initialises the state machine. */" << std::endl;
    out_h << "void " << collection->get_model_name() << "_init(" << styler->get_handle_type() << " handle);" << std::endl << std::endl;

    // write time tick function
    proto_timeTick();

    // write all raise event function prototypes
    proto_raiseInEvent();
    proto_checkOutEvent();

    proto_getVariable();

    // write header to .c
    out_c << getIndent(0) << "#include \"" << collection->get_model_name() << ".h\"" << std::endl;
    out_c << getIndent(0) << "#include <stdint.h>" << std::endl;
    out_c << getIndent(0) << "#include <stdbool.h>" << std::endl;
    out_c << getIndent(0) << "#include <stddef.h>" << std::endl;
    collection->stream_imports(out_c, "");
    out_c << std::endl;

    // declare prototypes for global run cycle, clear in events, etc.
    out_c << "static void " << styler->get_top_run_cycle() << "(" << styler->get_handle_type() << " handle);" << std::endl;
    proto_clearEvents();
    proto_runCycle();
    proto_entryAction();
    proto_exitAction();
    proto_raiseOutEvent();
    proto_raiseInternalEvent();

    // find first state on init
    const std::vector<State> firstState = collection->find_path_to_first_state();

    // write init implementation
    impl_init(firstState);

    // write all raise event functions
    impl_raiseInEvent();
    impl_checkOutEvent();
    impl_getVariable();
    impl_timeTick();
    impl_clearEvents();
    impl_topRunCycle();
    impl_runCycle();
    impl_entryAction();
    impl_exitAction();
    impl_raiseOutEvent();
    impl_raiseInternalEvent();

    /* end with a new line. */
    out_c << std::endl;
    out_h << std::endl;

    /* close streams. */
    out_c.close();
    out_h.close();
}

void Writer::errorReport(std::string str, unsigned int line)
{
    std::string fname = __FILE__;
    const size_t last_slash = fname.find_last_of("/");
    std::cout << "ERR: " << str << " - " << fname.substr(last_slash+1) << ": " << line << std::endl;
}

void Writer::proto_runCycle()
{
    for (auto s : collection->states)
    {
        if (("initial" == s.name) || ("final" == s.name))
        {
            // no run cycle for initial or final states.
        }
        else if (s.is_choice)
        {
            // no run cycle for choices.
        }
        else
        {
            out_c << getIndent(0) << "static bool " << styler->get_state_run_cycle(s) << "(" << styler->get_handle_type() << " handle, const bool try_transition);" << std::endl;
        }
    }
    out_c << std::endl;
}

void Writer::proto_entryAction()
{
    if (verbose)
    {
        std::cout << "Writing prototypes for entry actions." << std::endl;
    }

    for (auto s : collection->states)
    {
        if (("inital" != s.name) && (collection->has_entry_statement(s)))
        {
            out_c << getIndent(0) << "static void " << styler->get_state_entry(s) << "(" << styler->get_handle_type() << " handle);" << std::endl;
        }
    }
    out_c << std::endl;
}

void Writer::proto_exitAction()
{
    if (verbose)
    {
        std::cout << "Writing prototypes for exit actions." << std::endl;
    }

    for (auto s : collection->states)
    {
        if (("initial" != s.name) && (collection->has_exit_statement(s)))
        {
            out_c << getIndent(0) << "static void " << styler->get_state_exit(s) << "(" << styler->get_handle_type() << " handle);" << std::endl;
        }
    }
    this->out_c << std::endl;
}

void Writer::proto_raiseInEvent()
{
    bool did_write = false;
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && (EventDirection::Incoming == e.direction) && ("null" != e.name))
        {
            did_write = true;
            out_h << getIndent(0) << "void " << styler->get_event_raise(e) << "(" << styler->get_handle_type() << " handle";
            if (e.require_parameter)
            {
                out_h << ", const " << e.parameter_type << " value";
            }
            out_h << ");" << std::endl;
        }
    }

    if (did_write)
    {
        out_h << std::endl;
    }
}

void Writer::proto_raiseOutEvent()
{
    bool did_write = false;
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && (EventDirection::Outgoing == e.direction))
        {
            did_write = true;
            out_c << getIndent(0) << "static void " << styler->get_event_raise(e) << "(" << styler->get_handle_type() << " handle";
            if (e.require_parameter)
            {
                out_c << ", const " << e.parameter_type << " value";
            }
            out_c << ");" << std::endl;
        }
    }

    if (did_write)
    {
        out_c << std::endl;
    }
}

void Writer::proto_raiseInternalEvent()
{
    bool did_write = false;
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && (EventDirection::Internal == e.direction))
        {
            did_write = true;
            out_c << getIndent(0) << "static void " << styler->get_event_raise(e) << "(" << styler->get_handle_type() << " handle";
            if (e.require_parameter)
            {
                out_c << ", const " << e.parameter_type << " value";
            }
            out_c << ");" << std::endl;
        }
    }

    if (did_write)
    {
        out_c << std::endl;
    }
}

void Writer::proto_timeTick()
{
    bool did_write = false;
    if (styler->get_config_use_builtin_time_event_handling())
    {
        for (auto e : collection->events)
        {
            if (e.is_time_event)
            {
                did_write = true;
                    out_h << getIndent(0)
                          << "void "
                          << styler->get_time_tick()
                          << "("
                          << styler->get_handle_type()
                          << " handle, const size_t timeElapsed_ms);"
                          << std::endl;
            }
        }
    }
    else
    {
        if (!collection->events.empty())
        {
            did_write = true;
            out_h << getIndent(0)
                  << "void "
                  << styler->get_time_event_raise()
                  << "("
                  << styler->get_handle_type()
                  << " handle, const " << collection->get_model_name() << "_EventId_t event);"
                  << std::endl;
        }
    }

    if (did_write)
    {
        out_h << std::endl;
    }
}

void Writer::proto_checkOutEvent()
{
    bool did_write = false;
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && (EventDirection::Outgoing == e.direction))
        {
            did_write = true;
            out_h << "bool " << styler->get_event_is_raised(e) << "(const " << styler->get_handle_type() << " handle);" << std::endl;
        }
    }

    if (did_write)
    {
        out_h << std::endl;
    }
}

void Writer::proto_clearEvents(void)
{
    bool has_in_events = false;
    bool has_out_events = false;
    bool has_time_events = false;

    for (auto e : collection->events)
    {
        if (e.is_time_event)
        {
            has_time_events = true;
        }
        else
        {
            if (EventDirection::Incoming == e.direction)
            {
                has_in_events = true;
            }
            else if (EventDirection::Outgoing == e.direction)
            {
                has_out_events = true;
            }
        }
    }

    if (has_in_events)
    {
        out_c << "static void clearInEvents(" << styler->get_handle_type() << " handle);" << std::endl;
    }

    if (has_time_events)
    {
        out_c << "static void clearTimeEvents(" << styler->get_handle_type() << " handle);" << std::endl;
    }

    if (has_out_events)
    {
        out_c << "static void clearOutEvents(" << styler->get_handle_type() << " handle);" << std::endl;
    }

    if (has_in_events || has_out_events || has_time_events)
    {
        out_c << std::endl;
    }
}

void Writer::proto_getVariable(void)
{
    bool did_write = false;
    for (auto v : collection->variables)
    {
        if (!v.is_private)
        {
            did_write = true;
            out_h << v.type << " " << styler->get_variable(v) << "(const " << styler->get_handle_type() << " handle);" << std::endl;
        }
    }

    if (did_write)
    {
        out_h << std::endl;
    }
}

void Writer::proto_traceEntry(void)
{
    out_h << "extern void " << styler->get_trace_entry() << "(const " << styler->get_state_type() << " state);" << std::endl;
}

void Writer::proto_traceExit(void)
{
    out_h << "extern void " << styler->get_trace_exit() << "(const " << styler->get_state_type() << " state);" << std::endl;
}

void Writer::proto_startTimer()
{
    out_h << "extern void " << styler->get_start_timer() << "(" << styler->get_handle_type() << " handle, const " << collection->get_model_name() << "_EventId_t event, const size_t time_ms, const bool is_periodic);" << std::endl;
}

void Writer::proto_stopTimer()
{
    out_h << "extern void " << styler->get_stop_timer() << "(" << styler->get_handle_type() << " handle, const " << collection->get_model_name() << "_EventId_t event);" << std::endl;
}

void Writer::decl_types()
{
    out_h << getIndent(0) << "typedef uint8_t " << collection->get_model_name() << "_Bool_t;" << std::endl;
    out_h << getIndent(0) << "typedef void* " << collection->get_model_name() << "_EventId_t;" << std::endl << std::endl;
}

void Writer::decl_stateList()
{
    out_h << getIndent(0) << "typedef enum" << std::endl;
    out_h << getIndent(0) << "{" << std::endl;
    for (auto s : collection->states)
    {
        if (("initial" != s.name) && ("final" != s.name) && (!s.is_choice))
        {
            out_h << getIndent(1) << styler->get_state_name(s) << "," << std::endl;
        }
    }
    out_h << getIndent(0) << "} " << collection->get_model_name() << "_State_t;" << std::endl << std::endl;
}

void Writer::decl_eventList(void)
{
    bool has_in_events = false;
    bool has_out_events = false;
    bool has_time_events = false;
    bool has_internal_events = false;

    for (auto e : collection->events)
    {
        if (e.is_time_event)
        {
            has_time_events = true;
        }
        else
        {
            if (EventDirection::Incoming == e.direction)
            {
                has_in_events = true;
            }
            else if (EventDirection::Outgoing == e.direction)
            {
                has_out_events = true;
            }
            else if (EventDirection::Internal == e.direction)
            {
                has_internal_events = true;
            }
        }
    }

    if (!has_in_events && !has_out_events && !has_time_events && !has_internal_events)
    {
        // don't write any
    }
    else
    {
        if (has_time_events)
        {
            out_h << getIndent(0) << "typedef struct" << std::endl;
            out_h << getIndent(0) << "{" << std::endl;
            out_h << getIndent(1) << "bool isStarted;" << std::endl;
            out_h << getIndent(1) << "bool isRaised;" << std::endl;
            out_h << getIndent(1) << "bool isPeriodic;" << std::endl;
            out_h << getIndent(1) << "size_t timeout_ms;" << std::endl;
            out_h << getIndent(1) << "size_t expireTime_ms;" << std::endl;
            out_h << getIndent(0) << "} " << collection->get_model_name() << "_TimeEvent_t;" << std::endl << std::endl;
        }

        out_h << getIndent(0) << "typedef struct" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;

        if (has_in_events)
        {
            out_h << getIndent(1) << "struct" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (auto e : collection->events)
            {
                if ((!e.is_time_event) && (EventDirection::Incoming == e.direction) && ("null" != e.name))
                {
                    out_h << getIndent(2) << "bool " << e.name << "_isRaised;" << std::endl;
                    if (e.require_parameter)
                    {
                        out_h << getIndent(2) << e.parameter_type << " " << e.name << "_value;" << std::endl;
                    }
                }
            }
            out_h << getIndent(1) << "} inEvents;" << std::endl;
        }

        if (has_out_events)
        {
            out_h << getIndent(1) << "struct" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (auto e : collection->events)
            {
                if ((!e.is_time_event) && EventDirection::Outgoing == e.direction)
                {
                    out_h << getIndent(2) << "bool " << e.name << "_isRaised;" << std::endl;
                    if (e.require_parameter)
                    {
                        out_h << getIndent(2) << e.parameter_type << " " << e.name << "_value;" << std::endl;
                    }
                }
            }
            out_h << getIndent(1) << "} outEvents;" << std::endl;
        }

        if (has_time_events)
        {
            out_h << getIndent(1) << "struct" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (auto e : collection->events)
            {
                if (e.is_time_event)
                {
                    out_h << getIndent(2) << collection->get_model_name() << "_TimeEvent_t " << e.name << ";" << std::endl;
                }
            }
            out_h << getIndent(1) << "} timeEvents;" << std::endl;
        }

        if (has_internal_events)
        {
            out_h << getIndent(1) << "struct" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (auto e : collection->events)
            {
                if ((!e.is_time_event) && (EventDirection::Internal == e.direction))
                {
                    out_h << getIndent(2) << "bool " << e.name << "_isRaised;" << std::endl;
                }
            }
            out_h << getIndent(1) << "} internalEvents;" << std::endl;
        }
        out_h << getIndent(0) << "} " << collection->get_model_name() << "_EventList_t;" << std::endl;
        out_h << std::endl;
    }
}

void Writer::decl_variableList(void)
{
    bool has_private_variables = false;
    bool has_public_variables = false;

    for (auto v : collection->variables)
    {
        if (v.is_private)
        {
            has_private_variables = true;
        }
        else
        {
            has_public_variables = true;
        }
    }

    if (!has_public_variables && !has_private_variables)
    {
        // no variables!
    }
    else
    {
        out_h << getIndent(0) << "typedef struct" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;

        if (has_private_variables)
        {
            out_h << getIndent(1) << "struct" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (auto v : collection->variables)
            {
                if (v.is_private)
                {
                    out_h << getIndent(2) << v.type << " " << v.name << ";" << std::endl;
                }
            }
            out_h << getIndent(1) << "} internal;" << std::endl;
        }

        if (has_public_variables)
        {
            out_h << getIndent(1) << "struct" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (auto v : collection->variables)
            {
                if (!v.is_private)
                {
                    out_h << getIndent(2) << v.type << " " << v.name << ";" << std::endl;
                }
            }
            out_h << getIndent(1) << "} exported;" << std::endl;
        }

        out_h << getIndent(0) << "} " << collection->get_model_name() << "_Variables_t;" << std::endl << std::endl;
    }
}

void Writer::decl_stateMachine(void)
{
    // write internal structure
    out_h << getIndent(0) << "typedef struct" << std::endl;
    out_h << getIndent(0) << "{" << std::endl;
    out_h << getIndent(1) << collection->get_model_name() << "_State_t state;" << std::endl;
    out_h << getIndent(1) << collection->get_model_name() << "_EventList_t events;" << std::endl;
    out_h << getIndent(1) << collection->get_model_name() << "_Variables_t variables;" << std::endl;

    bool has_time_events = false;
    for (auto e : collection->events)
    {
        if (e.is_time_event)
        {
            has_time_events = true;
        }
    }
    if (has_time_events && styler->get_config_use_builtin_time_event_handling())
    {
        out_h << getIndent(1) << "size_t timeNow_ms;" << std::endl;
    }

    out_h << getIndent(0) << "} " << collection->get_model_name() << "_t;" << std::endl << std::endl;

    out_h << "typedef " << collection->get_model_name() << "_t* " << styler->get_handle_type() << ";" << std::endl << std::endl;
}

void Writer::impl_init(const std::vector<State> first_state_path)
{
    out_c << "void " << collection->get_model_name() << "_init(" << styler->get_handle_type() << " handle)" << std::endl;
    out_c << "{" << std::endl;

    // write variable inits
    out_c << getIndent(1) << "/* Initialise variables. */" << std::endl;
    for (auto v : collection->variables)
    {
        if (v.is_private)
        {
            out_c << getIndent(1) << "handle->variables.internal.";
        }
        else
        {
            out_c << getIndent(1) << "handle->variables.exported.";
        }
        out_c << v.name << " = ";
        if (v.has_specific_initial_value)
        {
            out_c << v.initial_value << ";" << std::endl;
        }
        else
        {
            out_c << "0;" << std::endl;
        }
    }
    out_c << std::endl;

    // write event clearing
    out_c << getIndent(1) << "/* Clear events. */" << std::endl;
    out_c << getIndent(1) << "clearInEvents(handle);" << std::endl;

    // super step to first state.
    if (!first_state_path.empty())
    {
        out_c << std::endl;
        out_c << getIndent(1) << "/* Set initial state. */" << std::endl;

        for (auto s : first_state_path)
        {
            if (collection->has_entry_statement(s))
            {
                // write entry action call
                out_c << getIndent(1) << styler->get_state_entry(s) << "(handle);" << std::endl;
            }
        }
        out_c << getIndent(1) << "handle->state = " << styler->get_state_name(first_state_path.back()) << ";" << std::endl;

        if (enable_tracing)
        {
            out_c << getIndent(1) << get_trace_call_entry(first_state_path.back()) << std::endl;
        }
    }
    out_c << "}" << std::endl << std::endl;
}

void Writer::impl_raiseInEvent()
{
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && (EventDirection::Incoming == e.direction) && ("null" != e.name))
        {
            out_c << getIndent(0) << "void " << styler->get_event_raise(e) << "(" << styler->get_handle_type() << " handle";
            if (e.require_parameter)
            {
                out_c << ", const " << e.parameter_type << " value";
            }
            out_c << ")" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            if (e.require_parameter)
            {
                out_c << getIndent(1) << "handle->events.inEvents." << e.name << "_value = value;" << std::endl;
            }
            out_c << getIndent(1) << "handle->events.inEvents." << e.name << "_isRaised = true;" << std::endl;
            out_c << getIndent(1) << styler->get_top_run_cycle() << "(handle);" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_raiseOutEvent(void)
{
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && EventDirection::Outgoing == e.direction)
        {
            out_c << getIndent(0) << "static void " << styler->get_event_raise(e) << "(" << styler->get_handle_type() << " handle";
            if (e.require_parameter)
            {
                out_c << ", const " << e.parameter_type << " value";
            }
            out_c << ")" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            if (e.require_parameter)
            {
                out_c << getIndent(1) << "handle->events.outEvents." << e.name << "_value = value;" << std::endl;
            }
            out_c << getIndent(1) << "handle->events.outEvents." << e.name << "_isRaised = true;" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_raiseInternalEvent(void)
{
    for (auto e : collection->events)
    {
        if ((!e.is_time_event && EventDirection::Internal == e.direction))
        {
            out_c << getIndent(0) << "static void " << styler->get_event_raise(e) << "(" << styler->get_handle_type() << " handle";
            if (e.require_parameter)
            {
                out_c << ", const " << e.parameter_type << " value";
            }
            out_c << ")" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            if (e.require_parameter)
            {
                out_c << getIndent(1) << "handle->events.internalEvents." << e.name << "_value = value;" << std::endl;
            }
            out_c << getIndent(1) << "handle->events.internalEvents." << e.name << "_isRaised = true;" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_checkOutEvent(void)
{
    for (auto e : collection->events)
    {
        if ((!e.is_time_event) && (EventDirection::Outgoing == e.direction))
        {
            out_c << getIndent(0) << "bool " << styler->get_event_is_raised(e) << "(const " << styler->get_handle_type() << " handle)" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            out_c << getIndent(1) << "return (handle->events.outEvents." << e.name << "_isRaised);" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_getVariable(void)
{
    for (auto v : collection->variables)
    {
        if (!v.is_private)
        {
            out_c << getIndent(0) << v.type << " " << styler->get_variable(v) << "(const " << styler->get_handle_type() << " handle)" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            out_c << getIndent(1) << "return (handle->variables.exported." << v.name << ");" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_timeTick(void)
{
    bool has_time_events = false;
    for (auto e : collection->events)
    {
        if (e.is_time_event)
        {
            has_time_events = true;
        }
    }

    if (has_time_events)
    {
        if (styler->get_config_use_builtin_time_event_handling())
        {
            out_c << getIndent(0) << "void " << styler->get_time_tick() << "(" << styler->get_handle_type()
                  << " handle, const size_t timeElapsed_ms)" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            out_c << getIndent(1) << "handle->timeNow_ms += timeElapsed_ms;" << std::endl << std::endl;

            for (auto e : collection->events)
            {
                if (e.is_time_event)
                {
                    out_c << getIndent(1) << "if (handle->events.timeEvents." << e.name << ".isStarted)" << std::endl;
                    out_c << getIndent(1) << "{" << std::endl;
                    out_c << getIndent(2) << "if (handle->events.timeEvents." << e.name
                          << ".expireTime_ms <= handle->timeNow_ms)" << std::endl;
                    out_c << getIndent(2) << "{" << std::endl;
                    out_c << getIndent(3) << "handle->events.timeEvents." << e.name << ".isRaised = true;" << std::endl;
                    out_c << getIndent(3) << "if (handle->events.timeEvents." << e.name << ".isPeriodic)" << std::endl;
                    out_c << getIndent(3) << "{" << std::endl;
                    out_c << getIndent(4) << "handle->events.timeEvents." << e.name
                          << ".expireTime_ms += handle->events.timeEvents." << e.name << ".timeout_ms;" << std::endl;
                    out_c << getIndent(4) << "handle->events.timeEvents." << e.name << ".isStarted = true;"
                          << std::endl;
                    out_c << getIndent(3) << "}" << std::endl;
                    out_c << getIndent(3) << "else" << std::endl;
                    out_c << getIndent(3) << "{" << std::endl;
                    out_c << getIndent(4) << "handle->events.timeEvents." << e.name << ".isStarted = false;"
                          << std::endl;
                    out_c << getIndent(3) << "}" << std::endl;
                    out_c << getIndent(2) << "}" << std::endl;
                    out_c << getIndent(1) << "}" << std::endl;
                }
            }
            out_c << std::endl;
            out_c << getIndent(1) << styler->get_top_run_cycle() << "(handle);" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
        else
        {
            out_c << getIndent(0) << "void " << styler->get_time_event_raise() << "(" << styler->get_handle_type() << " handle, const " << collection->get_model_name() << "_EventId_t event)" << std::endl;
            out_c << getIndent(0) << "{" << std::endl;
            unsigned int i = 0;
            for (auto e : collection->events)
            {
                if (e.is_time_event)
                {
                    out_c << getIndent(1) << get_if_else_if(i++) << " (event == &handle->events.timeEvents." << e.name << ")" << std::endl;
                    out_c << getIndent(1) << "{" << std::endl;
                    out_c << getIndent(2) << "/* Raise event and perform run-cycle. */" << std::endl;
                    out_c << getIndent(2) << "handle->events.timeEvents." << e.name << ".isRaised = true;" << std::endl;
                    out_c << getIndent(1) << "}" << std::endl;
                }
            }
            out_c << std::endl;
            out_c << getIndent(1) << styler->get_top_run_cycle() << "(handle);" << std::endl;
            out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_clearEvents(void)
{
    bool has_in_events = false;
    bool has_out_events = false;
    bool has_time_events = false;

    for (auto e : collection->events)
    {
        if (e.is_time_event)
        {
            has_time_events = true;
        }
        else
        {
            if (EventDirection::Incoming == e.direction)
            {
                has_in_events = true;
            }
            else if (EventDirection::Outgoing == e.direction)
            {
                has_out_events = true;
            }
        }
    }

    if (has_in_events)
    {
        out_c << "static void clearInEvents(" << styler->get_handle_type() << " handle)" << std::endl;
        out_c << "{" << std::endl;
        for (auto e : collection->events)
        {
            if ((!e.is_time_event) && (EventDirection::Incoming == e.direction))
            {
                out_c << getIndent(1) << "handle->events.inEvents." << e.name << "_isRaised = false;" << std::endl;
            }
        }
        out_c << "}" << std::endl << std::endl;
    }

    if (has_time_events)
    {
        out_c << "static void clearTimeEvents(" << styler->get_handle_type() << " handle)" << std::endl;
        out_c << "{" << std::endl;
        for (auto e : collection->events)
        {
            if (e.is_time_event)
            {
                out_c << getIndent(1) << "handle->events.timeEvents." << e.name << ".isRaised = false;" << std::endl;
            }
        }
        out_c << "}" << std::endl << std::endl;
    }

    if (has_out_events)
    {
        out_c << "static void clearOutEvents(" << styler->get_handle_type() << " handle)" << std::endl;
        out_c << "{" << std::endl;
        for (auto e : collection->events)
        {
            if ((!e.is_time_event) && (EventDirection::Outgoing == e.direction))
            {
                out_c << getIndent(1) << "handle->events.outEvents." << e.name << "_isRaised = false;" << std::endl;
            }
        }
        out_c << "}" << std::endl << std::endl;
    }
}


void Writer::impl_topRunCycle(void)
{
    bool has_in_events = false;
    bool has_out_events = false;
    bool has_time_events = false;
    bool has_internal_events = false;

    for (auto e : collection->events)
    {
        if (e.is_time_event)
        {
            has_time_events = true;
        }
        else
        {
            if (EventDirection::Incoming == e.direction)
            {
                has_in_events = true;
            }
            else if (EventDirection::Outgoing == e.direction)
            {
                has_out_events = true;
            }
            else if (EventDirection::Internal == e.direction)
            {
                has_internal_events = true;
            }
        }
    }

    unsigned int write_number = 0;
    unsigned int indent = 0;

    out_c << "static void " << styler->get_top_run_cycle() << "(" << styler->get_handle_type() << " handle)" << std::endl;
    out_c << "{" << std::endl;
    indent++;

    if (has_out_events)
    {
        out_c << getIndent(indent) << "clearOutEvents(handle);" << std::endl << std::endl;
    }

    if (has_internal_events)
    {
        out_c << getIndent(indent) << "/* While an internal event is raised, perform loop. */" << std::endl;
        out_c << getIndent(indent) << "do" << std::endl;
        out_c << getIndent(indent) << "{" << std::endl;
        indent++;
    }

    for (auto s : collection->states)
    {
        if (("initial" == s.name) || ("final" == s.name))
        {
            // no handling on initial or final states.
        }
        else if (s.is_choice)
        {
            // no handling on choice.
        }
        else
        {
            out_c << getIndent(indent) << get_if_else_if(write_number) << " (handle->state == " << styler->get_state_name(s) << ")" << std::endl;
            out_c << getIndent(indent) << "{" << std::endl;
            indent++;
            out_c << getIndent(indent) << styler->get_state_run_cycle(s) << "(handle, true);" << std::endl;
            indent--;
            out_c << getIndent(indent) << "}" << std::endl;
            write_number++;
        }
    }

    if (!collection->states.empty())
    {
        out_c << getIndent(indent) << "else" << std::endl;
        out_c << getIndent(indent) << "{" << std::endl;
        indent++;
        out_c << getIndent(indent) << "/* Possibly final state. */" << std::endl;
        indent--;
        out_c << getIndent(indent) << "}" << std::endl;
    }

    // TODO: Check for pending internal events, then do another run..
    if (has_internal_events)
    {
        indent--;
        out_c << getIndent(indent) << "} while (false);" << std::endl << std::endl;
    }
    else
    {
        out_c << std::endl;
    }

    if (has_in_events)
    {
        out_c << getIndent(indent) << "clearInEvents(handle);" << std::endl;
    }

    if (has_time_events)
    {
        out_c << getIndent(indent) << "clearTimeEvents(handle);" << std::endl;
    }

    out_c << "}" << std::endl << std::endl;
}

void Writer::impl_runCycle(void)
{
    for (auto s : collection->states)
    {
        if (("initial" == s.name) || ("final" == s.name))
        {
            // initial or final state, no runcycle
        }
        else if (s.is_choice)
        {
            // no run cycle for choice
        }
        else
        {
            unsigned int indent = 0;
            bool is_body_empty = true;

            out_c << "static bool " << styler->get_state_run_cycle(s) << "(" << styler->get_handle_type() << " handle, const bool try_transition)" << std::endl;
            out_c << "{" << std::endl;
            indent++;

            // write comment declaration here if one exists.
            std::vector<State_Decl> state_comments = collection->get_state_declarations(s, Declaration::Comment);
            if (!state_comments.empty())
            {
                is_body_empty = false;

                for (auto d : state_comments)
                {
                    out_c << getIndent(indent) << "/* " << d.content << " */" << std::endl;
                }

                out_c << std::endl;
            }

            out_c << getIndent(indent) << "bool did_transition = try_transition;" << std::endl;
            out_c << getIndent(indent) << "if (try_transition)" << std::endl;
            out_c << getIndent(indent) << "{" << std::endl;
            indent++;

            // write parent react
            State parent = collection->get_state_by_id(s.parent);
            if (0 < parent.id)
            {
                is_body_empty = false;
                {
                    out_c << getIndent(indent) << "if (!" << styler->get_state_run_cycle(parent) << "(handle, try_transition))" << std::endl;
                    out_c << getIndent(indent) << "{" << std::endl;
                    indent++;
                }
            }

            std::vector<Transition> transitions = collection->get_transitions_from_state(s);
            if (transitions.empty())
            {
                out_c << getIndent(indent) << "did_transition = false;" << std::endl;
            }
            else
            {
                unsigned int tr_index = 0;
                for (auto t : transitions)
                {
                    if ("null" == t.event.name)
                    {
                        State tr_st_b = collection->get_state_by_id(t.state_b);
                        if (0 == tr_st_b.id)
                        {
                            errorReport("Null transition!", __LINE__);
                        }
                        else if ("final" != tr_st_b.name)
                        {
                            errorReport("Null transition!", __LINE__);

                            // handle as a oncycle transition?
                            is_body_empty = false;

                            out_c << getIndent(indent) << get_if_else_if(tr_index) << " (true)" << std::endl;
                            out_c << getIndent(indent) << "{" << std::endl;
                            indent++;

                            // if tracing is enabled
                            if (enable_tracing)
                            {
                                out_c << getIndent(indent) << get_trace_call_exit(tr_st_b) << std::endl;
                            }

                            // is exit function exists
                            if (collection->has_exit_statement(s))
                            {
                                out_c << getIndent(indent) << styler->get_state_exit(s) << "(handle);" << std::endl;
                            }

                            indent--;
                            out_c << getIndent(indent) << "}" << std::endl;
                        }
                    }
                    else
                    {
                        State tr_st_b = collection->get_state_by_id(t.state_b);
                        if (0 == tr_st_b.id)
                        {
                            errorReport("Null transition!", __LINE__);
                        }
                        else
                        {
                            is_body_empty = false;

                            if (t.event.is_time_event)
                            {
                                if (t.has_guard)
                                {
                                    out_c << getIndent(indent)
                                          << get_if_else_if(tr_index)
                                          << " ((handle->events.timeEvents."
                                          << t.event.name
                                          << ".isRaised) && ("
                                          << parse_guard(t.guard_content)
                                          << "))"
                                          << std::endl;
                                }
                                else
                                {
                                    out_c << getIndent(indent)
                                          << get_if_else_if(tr_index)
                                          << " (handle->events.timeEvents."
                                          << t.event.name
                                          << ".isRaised)"
                                          << std::endl;
                                }
                            }
                            else
                            {
                                if (t.has_guard)
                                {
                                    out_c << getIndent(indent) << get_if_else_if(tr_index);

                                    if (EventDirection::Incoming == t.event.direction)
                                    {
                                        out_c << " ((handle->events.inEvents."
                                              << t.event.name
                                              << "_isRaised) && (";
                                    }
                                    else if (EventDirection::Internal == t.event.direction)
                                    {
                                        out_c << " ((handle->events.internalEvents."
                                              << t.event.name
                                              << "_isRaised) && (";
                                    }
                                    else
                                    {
                                        out_c << " ((handle->events.outEvents."
                                              << t.event.name
                                              << "_isRaised) && (";
                                    }
                                    out_c << parse_guard(t.guard_content) << "))" << std::endl;
                                }
                                else
                                {
                                    if (EventDirection::Incoming == t.event.direction)
                                    {
                                        out_c << getIndent(indent) << get_if_else_if(tr_index) << " (handle->events.inEvents." << t.event.name << "_isRaised)" << std::endl;
                                    }
                                    else if (EventDirection::Internal == t.event.direction)
                                    {
                                        out_c << getIndent(indent) << get_if_else_if(tr_index) << " (handle->events.internalEvents." << t.event.name << "_isRaised)" << std::endl;
                                    }
                                    else
                                    {
                                        out_c << getIndent(indent) << get_if_else_if(tr_index) << " (handle->events.outEvents." << t.event.name << "_isRaised)" << std::endl;
                                    }
                                }
                            }
                            out_c << getIndent(indent) << "{" << std::endl;
                            indent++;

                            /// Better?
                            /// 1) Find first common parent.
                            /// 2) Get path from current to common parent.
                            /// 3) Get path from target to common parent.

                            unsigned int current_parent = s.parent;
                            unsigned int common_parent = 0;

                            /* If state is not in parent, then of course the common is 0. */
                            while ((0 != current_parent) && (0 == common_parent))
                            {
                                /* Step up target until common found. */
                                unsigned int target_parent = tr_st_b.parent;
                                while ((0 != target_parent) && (0 == common_parent))
                                {
                                    if (target_parent == current_parent)
                                    {
                                        common_parent = target_parent;
                                    }
                                    else
                                    {
                                        target_parent = collection->get_state_by_id(target_parent).parent;
                                    }
                                }
                                current_parent = collection->get_state_by_id(current_parent).parent;
                            }

                            std::cout << "Common parent: " << common_parent << std::endl;

                            std::vector<State> path_out {};
                            //std::vector<State> path_in_tmp {};

                            State common_state = s;

                            do
                            {
                                path_out.push_back(common_state);
                                common_state = collection->get_state_by_id(common_state.parent);
                            } while (common_state.id != common_parent);

#if 0
                            State target_state = tr_st_b;

                            while (target_state.id != common_state.id)
                            {
                                path_in_tmp.push_back(target_state);
                                target_state = collection->get_state_by_id(target_state.parent);
                            };

                            /* Check if entry to target actually leads somewhere else.. */
                            auto path_addon = collection->find_entry_state(target_state);
                            std::vector<State> path_in {};
                            if (!path_in_tmp.empty())
                            {
                                for (auto it = path_in_tmp.end(); it != path_in_tmp.begin();)
                                {
                                    it--;

                                    path_in.push_back(*it);
                                }
                            }
                            for (auto _s : path_addon)
                            {
                                path_in.push_back(_s);
                            }
#endif
                            auto path_in = collection->find_entry_state(tr_st_b);

                            /* Here, we should have a list of states to exit and then enter.. right? */
                            if (!path_out.empty())
                            {
                                //out_c << getIndent(indent) << "/* Handle super-step exit. */" << std::endl;
                                for (auto _s : path_out)
                                {
                                    if (collection->has_exit_statement(_s))
                                    {
                                        out_c << getIndent(indent) << styler->get_state_exit(_s) << "(handle);" << std::endl;
                                    }
                                    if (enable_tracing && ("initial" != _s.name))
                                    {
                                        out_c << getIndent(indent) << get_trace_call_exit(_s) << std::endl;
                                    }
                                }
                            }
                            if (!path_in.empty())
                            {
                                //out_c << getIndent(indent) << "/* Handle super-step entry. */" << std::endl;
                                for (auto _s : path_in)
                                {
                                    if (collection->has_entry_statement(_s))
                                    {
                                        out_c << getIndent(indent) << styler->get_state_entry(_s) << "(handle);" << std::endl;
                                    }
                                    if (enable_tracing && ("initial" != _s.name))
                                    {
                                        out_c << getIndent(indent) << get_trace_call_entry(_s) << std::endl;
                                    }
                                }
                            }


#if 0
                            if (parse_child_exits(s, indent, s, false))
                            {
                                out_c << std::endl;
                            }
                            else
                            {
                                bool did_write = false;
                                if (collection->has_exit_statement(s))
                                {
                                    did_write = true;
                                    out_c << getIndent(indent) << "/* Handle super-step exit. */" << std::endl;
                                    out_c << getIndent(indent) << styler->get_state_exit(s) << "(handle);" << std::endl;
                                }

                                if (enable_tracing)
                                {
                                    did_write = true;
                                    //out_c << getIndent(indent) << "/* Trace state exit. */" << std::endl;
                                    out_c << getIndent(indent) << get_trace_call_exit(s) << std::endl;
                                }

                                /* Extra new-line */
                                if (did_write)
                                {
                                    out_c << std::endl;
                                }
                            }

                            // TODO: perform transition actions.
                            if (!t.action_content.empty())
                            {
                                out_c << getIndent(indent) << "/* Perform transition actions. */" << std::endl;
                                // todo: fix the parser to return a string instead...
                                for (auto item : t.action_content)
                                {
                                    out_c << getIndent(indent-1);
                                    parse_declaration(item);
                                }
                                out_c << std::endl;
                            }

                            // TODO: do entry actions on all states entered
                            // towards the goal! Might needs some work..
                            std::vector<State> entered_states = collection->find_entry_state(tr_st_b);

                            if (!entered_states.empty())
                            {
                                out_c << getIndent(indent) << "/* Handle super-step entry. */" << std::endl;
                            }

                            State final_state {};
                            for (auto es : entered_states)
                            {
                                final_state = es;

                                if (collection->has_entry_statement(final_state))
                                {
                                    out_c << getIndent(indent) << styler->get_state_entry(final_state) << "(handle);" << std::endl;
                                }

                                if (enable_tracing)
                                {
                                    // Don't trace entering the choice states, since the state does not exist.
                                    if (!final_state.is_choice)
                                    {
                                        out_c << getIndent(indent) << this->get_trace_call_entry(final_state) << std::endl;
                                    }
                                }
                            }

                            // handle choice node?
                            if ((0 != final_state.id) && (final_state.is_choice))
                            {
                                parse_choice_path(final_state, indent);
                            }
                            else
                            {
                                out_c << getIndent(indent) << "handle->state = " << styler->get_state_name(final_state) << ";" << std::endl;
                            }
#endif

                            indent--;
                            out_c << getIndent(indent) << "}" << std::endl;
                        }
                    }

                    tr_index++;
                }

                out_c << getIndent(indent) << "else" << std::endl;
                out_c << getIndent(indent) << "{" << std::endl;
                indent++;
                out_c << getIndent(indent) << "did_transition = false;" << std::endl;
                indent--;
                out_c << getIndent(indent) << "}" << std::endl;
            }

            while (1 < indent)
            {
                indent--;
                out_c << getIndent(indent) << "}" << std::endl;
            }

            if (is_body_empty)
            {
                out_c << getIndent(indent) << "(void) handle;" << std::endl;
            }

            out_c << getIndent(indent) << "return (did_transition);" << std::endl;
            out_c << "}" << std::endl << std::endl;
        }
    }
}

void Writer::impl_entryAction(void)
{
    for (auto s : collection->states)
    {
        if ("initial" != s.name)
        {
            std::vector<State_Decl> entry_declarations = collection->get_state_declarations(s, Declaration::Entry);
            std::vector<Transition> transitions = collection->get_transitions_from_state(s);

            unsigned int num_time_ev = 0;

            for (auto t : transitions)
            {
                if (t.event.is_time_event)
                {
                    num_time_ev++;
                }
            }

            if ((!entry_declarations.empty()) || (0 < num_time_ev))
            {
                out_c << getIndent(0) << "static void " << styler->get_state_entry(s) << "(" << styler->get_handle_type() << " handle)" << std::endl;
                out_c << getIndent(0) << "{" << std::endl;

                // start timers
                unsigned int write_index = 0;
                for (auto t : transitions)
                {
                    if (t.event.is_time_event)
                    {
                        out_c << getIndent(1) << "/* Start timer " << t.event.name << " with timeout of " << t.event.expire_time_ms << " ms. */" << std::endl;
                        if (styler->get_config_use_builtin_time_event_handling())
                        {
                            out_c << getIndent(1) << "handle->events.timeEvents." << t.event.name << ".timeout_ms = " << t.event.expire_time_ms << ";" << std::endl;
                            out_c << getIndent(1) << "handle->events.timeEvents." << t.event.name << ".expireTime_ms = handle->timeNow_ms + " << t.event.expire_time_ms << ";" << std::endl;
                            out_c << getIndent(1) << "handle->events.timeEvents." << t.event.name << ".isPeriodic = " << (t.event.is_periodic ? "true;" : "false;") << std::endl;
                            out_c << getIndent(1) << "handle->events.timeEvents." << t.event.name << ".isStarted = true;" << std::endl;
                        }
                        else
                        {
                            out_c << getIndent(1) << styler->get_start_timer() << "(handle, &handle->events.timeEvents." << t.event.name << ", " << t.event.expire_time_ms << ", " << (t.event.is_periodic ? "true" : "false") << ");" << std::endl;
                        }

                        write_index++;
                        if (write_index < num_time_ev)
                        {
                            out_c << std::endl;
                        }
                    }
                }

                if ((!entry_declarations.empty()) && (0 < num_time_ev))
                {
                    // add a space between the parts
                    out_c << std::endl;
                }

                for (auto d : entry_declarations)
                {
                    parse_declaration(d.content);
                }
                out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
}

void Writer::impl_exitAction(void)
{
    for (auto s : collection->states)
    {
        if ("initial" != s.name)
        {
            auto declarations = collection->get_state_declarations(s, Declaration::Exit);
            auto transitions = collection->get_transitions_from_state(s);

            unsigned int num_time_ev = 0;

            for (auto t : transitions)
            {
                if (t.event.is_time_event)
                {
                    num_time_ev++;
                }
            }

            if ((!declarations.empty()) || (0 < num_time_ev))
            {
                out_c << getIndent(0) << "static void " << styler->get_state_exit(s) << "(" << styler->get_handle_type() << " handle)" << std::endl;
                out_c << getIndent(0) << "{" << std::endl;

                // stop timers
                for (auto t : transitions)
                {
                    if (t.event.is_time_event)
                    {
                        if (styler->get_config_use_builtin_time_event_handling())
                        {
                            out_c << getIndent(1) << "handle->events.timeEvent." << t.event.name
                                  << ".isStarted = false;" << std::endl;
                        }
                        else
                        {
                            out_c << getIndent(1) << styler->get_stop_timer() << "(handle, &handle->events.timeEvents." << t.event.name << ");" << std::endl;
                        }
                    }
                }

                if ((!declarations.empty()) && (0 < num_time_ev))
                {
                    // add a space between the parts
                    out_c << std::endl;
                }

                if (!declarations.empty())
                {
                    for (auto d : declarations)
                    {
                        parse_declaration(d.content);
                    }
                }
                out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
}

std::vector<std::string> Writer::tokenize(const std::string& str) const
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

void Writer::parse_declaration(const std::string& declaration)
{
    // replace all X that corresponds with an event name with handle->events.X.param
    // also replace any word found that corresponds to a variable to its
    // correct location, e.g., bus -> handle->variables.private.bus
    std::string wstr = "";

    bool isParsed = false;
    size_t start = 0;
    while (!isParsed)
    {
        const size_t replaceStart = declaration.find_first_of("$", start);
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
            const size_t replaceEnd = declaration.find_first_of("}", replaceStart);
            if (std::string::npos == replaceEnd)
            {
                // error!
                std::cout << "Error: Invalid format of variable/event." << std::endl;
                isParsed = true;
            }
            else
            {
                const size_t replaceLength = (replaceEnd - replaceStart) - 2;
                const std::string replaceString = declaration.substr(replaceStart+2, replaceLength);
                bool isReplaced = false;

                for (auto v : collection->variables)
                {
                    if (replaceString == v.name)
                    {
                        wstr += "handle->variables.";
                        if (v.is_private)
                        {
                            wstr += "internal.";
                        }
                        else
                        {
                            wstr += "exported.";
                        }
                        wstr += v.name;
                        isReplaced = true;
                        break;
                    }
                }

                if (!isReplaced)
                {
                    for (auto e : collection->events)
                    {
                        if ((EventDirection::Incoming == e.direction) && (replaceString == e.name))
                        {
                            wstr += "handle->events.inEvents." + e.name + "_value";
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
    std::string outstr = "";
    isParsed = false;
    start = 0;
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
            const size_t firstSpacePosition = wstr.find_first_of(" ", raiseKeywordPosition);
            if (std::string::npos == firstSpacePosition)
            {
                // no space detected after the keyword
                outstr += wstr.substr(start);
                isParsed = true;
            }
            else
            {
                const size_t nextSpacePosition = wstr.find_first_of(" ", firstSpacePosition + 1);
                if (std::string::npos == nextSpacePosition)
                {
                    // no more space found, check length
                    if ((nextSpacePosition + 1) < wstr.length())
                    {
                        // take the last part as the outgoing event
                        std::string evName = wstr.substr(firstSpacePosition+1);
                        if (';' == evName.back())
                        {
                            evName.pop_back();
                        }
                        Event e {};
                        e.name = evName;
                        outstr += styler->get_event_raise(e) + "(handle);";
                    }
                    else
                    {
                        outstr += wstr.substr(start);
                    }
                    isParsed = true;
                }
                else
                {
                    // take the word between the spaces as the event
                    std::string evName = wstr.substr(firstSpacePosition + 1, (nextSpacePosition - firstSpacePosition) - 1);
                    if (';' == evName.back())
                    {
                        evName.pop_back();
                    }
                    outstr += "raise_" + evName + "(handle); ";
                    start = nextSpacePosition;
                }
            }
        }
    }

    if (';' != outstr.back())
    {
        outstr += ";";
    }

    out_c << getIndent(1) << outstr << std::endl;
}

std::string Writer::parse_guard(const std::string& guardStrRaw)
{
    // replace all X that corresponds with an event name with handle->events.X.param
    // also replace any word found that corresponds to a variable to its
    // correct location, e.g., bus -> handle->variables.private.bus
    std::string wstr = "";

    bool isParsed = false;
    size_t start = 0;
    while (!isParsed)
    {
        const size_t replaceStart = guardStrRaw.find_first_of("$", start);
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
            const size_t replaceEnd = guardStrRaw.find_first_of("}", replaceStart);
            if (std::string::npos == replaceEnd)
            {
                // error!
                std::cout << "Error: Invalid format of variable/event." << std::endl;
                isParsed = true;
            }
            else
            {
                const size_t replaceLength = (replaceEnd - replaceStart) - 2;
                const std::string replaceString = guardStrRaw.substr(replaceStart+2, replaceLength);
                bool isReplaced = false;

                for (auto v : collection->variables)
                {
                    if (replaceString == v.name)
                    {
                        wstr += "handle->variables.";
                        if (v.is_private)
                        {
                            wstr += "internal.";
                        }
                        else
                        {
                            wstr += "exported.";
                        }
                        wstr += v.name;
                        isReplaced = true;
                        break;
                    }
                }

                if (!isReplaced)
                {
                    for (auto e : collection->events)
                    {
                        if (replaceString == e.name)
                        {
                            wstr += "handle->events.inEvents." + e.name + ".param";
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

void Writer::parse_choice_path(State& initial_choice, unsigned int indent)
{
    // check all transitions from the choice..
    out_c << std::endl << getIndent(indent) << "/* Choice: " << initial_choice.name << " */" << std::endl;

    auto transitions = collection->get_transitions_from_state(initial_choice);
    if (transitions.size() < 2)
    {
        errorReport("Ony one transition from choice " + initial_choice.name, __LINE__);
    }
    else
    {
        Transition default_transition {};
        unsigned int k = 0;

        for (auto t : transitions)
        {
            if (!t.has_guard)
            {
                default_transition = t;
            }
            else
            {
                // handle if statement
                out_c << getIndent(indent + 0) << get_if_else_if(k++) << " (" << parse_guard(t.guard_content) << ")" << std::endl;
                out_c << getIndent(indent + 0) << "{" << std::endl;
                State guarded_state = collection->get_state_by_id(t.state_b);
                out_c << getIndent(indent + 1) << "// Goto: " << guarded_state.name << std::endl;
                if (0 == guarded_state.id)
                {
                    errorReport("Invalid transition from choice " + initial_choice.name, __LINE__);
                }
                else
                {
                    auto entered_states = collection->find_entry_state(guarded_state);
                    State final_state {};
                    for (auto s : entered_states)
                    {
                        final_state = s;

                        // TODO: fix call so send state name.
                        if (enable_tracing)
                        {
                            out_c << getIndent(indent + 1) << get_trace_call_entry(final_state) << std::endl;
                        }

                        if (collection->has_entry_statement(final_state))
                        //auto declarations = collection->get_state_declarations(final_state, Declaration::Entry);
                        //if (!declarations.empty())
                        {
                            out_c << getIndent(indent + 1) << styler->get_state_entry(final_state) << "(handle);" << std::endl;
                        }
                    }

                    if (0 != final_state.id)
                    {
                        if (final_state.is_choice)
                        {
                            // nest ..
                            parse_choice_path(final_state, indent + 1);
                        }
                        else
                        {
                            out_c << getIndent(indent + 1) << "handle->state = " << styler->get_state_name(final_state) << ";" << std::endl;
                        }
                    }
                }
                out_c << getIndent(indent + 0) << "}" << std::endl;
            }
        }

        if (0 != default_transition.id)
        {
            // write default transition.
            out_c << getIndent(indent + 0) << "else" << std::endl;
            out_c << getIndent(indent + 0) << "{" << std::endl;
            State guarded_state = collection->get_state_by_id(default_transition.state_b);
            out_c << getIndent(indent + 1) << "// Goto: " << guarded_state.name << std::endl;

            if (0 == guarded_state.id)
            {
                errorReport("Invalid transition from choice " + initial_choice.name, __LINE__);
            }
            else
            {
                auto entered_states = collection->find_entry_state(guarded_state);
                State final_state {};
                for (auto s : entered_states)
                {
                    final_state = s;

                    // TODO: fix call so send state name.
                    if (enable_tracing)
                    {
                        out_c << getIndent(indent + 1) << get_trace_call_entry(final_state) << std::endl;
                    }

                    if (collection->has_entry_statement(s))
                    //auto declarations = collection->get_state_declarations(s, Declaration::Entry);
                    //if (!declarations.empty())
                    {
                        out_c << getIndent(indent + 1) << styler->get_state_entry(final_state) << "(handle);" << std::endl;
                    }
                }

                if (0 != final_state.id)
                {
                    if (final_state.is_choice)
                    {
                        // nest ..
                        parse_choice_path(final_state, indent + 1);
                    }
                    else
                    {
                        out_c << getIndent(indent + 1) << "handle->state = " << styler->get_state_name(final_state) << ";" << std::endl;
                    }
                }
            }
            out_c << getIndent(indent + 0) << "}" << std::endl;
        }
        else
        {
            errorReport("No default transition from " + initial_choice.name, __LINE__);
        }
    }
}

std::vector<State> Writer::get_child_states(const State& current_state)
{
    std::vector<State> child_states;
    for (auto s : collection->states)
    {
        if ((0 != s.id) &&
            (s.parent == current_state.id) &&
            ("initial" != s.name) &&
            ("final" != s.name) &&
            (!s.is_choice))
        {
            child_states.push_back(s);
        }
    }
    return (child_states);
}

bool Writer::parse_child_exits(State current_state, unsigned int indent, const State& top_state, const bool did_previous_write)
{
    bool did_write = did_previous_write;

    auto children = get_child_states(current_state);

    if (children.empty())
    {
        // need to detect here if any state from current to top state has exit
        // actions..
        State tmp_state = current_state;
        bool has_exit_action = false;
        while (top_state.id != tmp_state.id)
        {
            if (collection->has_exit_statement(tmp_state))
            {
                has_exit_action = true;
                break;
            }
            tmp_state = collection->get_state_by_id(tmp_state.parent);
#if 0
            auto declarations = collection->get_state_declarations(tmp_state, Declaration::Exit);
            if (!declarations.empty())
            {
                has_exit_action = true;
                break;
            }
            tmp_state = collection->get_state_by_id(tmp_state.parent);
#endif
        }

        if (has_exit_action)
        {
            if (!did_write)
            {
                out_c << getIndent(indent) << "/* Handle super-step exit. */" << std::endl;
            }
            out_c << getIndent(indent) << get_if_else_if(did_write ? 1 : 0) << " (" << styler->get_state_name(current_state) << " == handle->state)" << std::endl;
            out_c << getIndent(indent) << "{" << std::endl;
            indent++;

            if (collection->has_exit_statement(current_state))
            //auto declarations = collection->get_state_declarations(current_state, Declaration::Exit);
            //if (!declarations.empty())
            {
                out_c << getIndent(indent) << styler->get_state_exit(current_state) << "(handle);" << std::endl;
            }

            if (enable_tracing)
            {
                out_c << getIndent(indent) << get_trace_call_exit(current_state) << std::endl;
            }

            // go up to the top
            while (top_state.id != current_state.id)
            {
                current_state = collection->get_state_by_id(current_state.parent);

                if (collection->has_exit_statement(current_state))
                //auto updec = collection->get_state_declarations(current_state, Declaration::Exit);
                //if (!updec.empty())
                {
                    out_c << getIndent(indent) << styler->get_state_exit(current_state) << "(handle);" << std::endl;
                }

                if (enable_tracing)
                {
                    out_c << getIndent(indent) << get_trace_call_exit(current_state) << std::endl;
                }
            }

            indent--;
            out_c << getIndent(indent) << "}" << std::endl;
            did_write = true;
        }
    }
    else
    {
        for (auto s : children)
        {
            did_write = parse_child_exits(s, indent, top_state, did_write);
        }
    }

    return (did_write);
}

std::string Writer::get_trace_call_entry(const State& state) const
{
    return (styler->get_trace_entry() + "(" + styler->get_state_name(state) + ");");
}

std::string Writer::get_trace_call_exit(const State& state) const
{
    return (styler->get_trace_exit() + "(" + styler->get_state_name(state) + ");");
}

std::string Writer::getIndent(const size_t level)
{
    std::string s = "";
    for (size_t i = 0; i < level; i++)
    {
        s += "    ";
    }
    return (s);
}

std::string Writer::get_if_else_if(const unsigned int i)
{
    return ((0 != i) ? "else if" : "if");
}

