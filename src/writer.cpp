/** @file
 *  @brief Implementation of the writer class.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include "../include/reader.hpp"
#include "../include/writer.hpp"

Writer_t::Writer_t(const std::string &filename, const std::string &outdir, const Writer_Config_t& cfg) :
    config(cfg), filename(filename), outdir(outdir), out_c(), out_h(), reader(filename, cfg.verbose), styler(reader)
{}

Writer_t::~Writer_t()
{
    if (out_c.is_open())
    {
        out_c.close();
    }
    if (out_h.is_open())
    {
        out_h.close();
    }
}

void Writer_t::generateCode()
{
    if (config.useSimpleNames)
    {
        styler.enableSimpleNames();
    }
    else
    {
        styler.disableSimpleNames();
    }

    auto model = reader.getModelName();
    if (!model.empty())
    {
        model[0] = static_cast<char>(std::tolower(model[0]));
    }

    const std::string outfile_c = outdir + model + ".cpp";
    const std::string outfile_h = outdir + model + ".h";

    if (config.verbose)
    {
        std::cout << "Generating code from '" << filename << "' > '" << outfile_c << "' and '" << outfile_h << "' ..." << std::endl;
    }

    out_c.open(outfile_c);
    out_h.open(outfile_h);

    if (!out_c.is_open() || !out_h.is_open())
    {
        errorReport("Failed to open output files, does directory exist?", __LINE__);
        return;
    }

    out_h << "/** @file" << std::endl;
    out_h << " *  @brief Interface to the " << reader.getModelName() << " state machine." << std::endl;
    out_h << " *" << std::endl;
    out_h << " *  @startuml" << std::endl;
    for (size_t i = 0; i < reader.getUmlLineCount(); i++)
    {
        out_h << " *  " << reader.getUmlLine(i) << std::endl;
    }
    out_h << " *  @enduml" << std::endl;
    out_h << " */" << std::endl << std::endl;

    out_h << getIndent(0) << "#include <cstdint>" << std::endl;
    out_h << getIndent(0) << "#include <cstddef>" << std::endl;
    out_h << getIndent(0) << "#include <functional>" << std::endl;
    out_h << getIndent(0) << "#include <deque>" << std::endl;
    out_h << getIndent(0) << "#include <string>" << std::endl;

    for (size_t i = 0; i < reader.getImportCount(); i++)
    {
        Import_t* imp = reader.getImport(i);
        out_h << getIndent(0) << "#include ";
        if (imp->isGlobal)
        {
            out_h << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            out_h << "\"" << imp->name << "\"" << std::endl;
        }
    }
    out_h << std::endl;

    // write all states to .h
    decl_stateList();

    // write all event types
    decl_eventList();

    // write all variables
    decl_variableList();

    // write tracing callback types
    decl_tracingCallback();

    // write main declaration
    decl_stateMachine();

    // write header to .c
    out_c << getIndent(0) << "#include \"" << model << ".h\"" << std::endl;

    for (size_t i = 0; i < reader.getImportCount(); i++)
    {
        Import_t* imp = reader.getImport(i);
        out_c << getIndent(0) << "#include ";
        if (imp->isGlobal)
        {
            out_c << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            out_c << "\"" << imp->name << "\"" << std::endl;
        }
    }
    out_c << std::endl;

    // find first state on init
    const std::vector<State_t*> firstState = findInitState();

    // write init implementation
    impl_init(firstState);

    // write trace calls wrappers
    impl_traceCalls();

    // write all raise event functions
    impl_raiseInEvent();
    impl_checkOutEvent();
    impl_getVariable();
    impl_timeTick();
    impl_clearEvents();
    impl_checkAnyEventRaised();
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

void Writer_t::errorReport(const std::string& str, unsigned int line)
{
    std::string fname = __FILE__;
    const size_t last_slash = fname.find_last_of("/");
    std::cout << "ERR: " << str << " - " << fname.substr(last_slash+1) << ": " << line << std::endl;
}

void Writer_t::proto_runCycle()
{
    out_c << std::endl;
}

void Writer_t::proto_entryAction()
{
}

void Writer_t::proto_exitAction()
{
}

void Writer_t::proto_raiseInEvent()
{
}

void Writer_t::proto_raiseOutEvent()
{
}

void Writer_t::proto_raiseInternalEvent()
{
}

void Writer_t::proto_timeTick()
{
}

void Writer_t::proto_checkOutEvent()
{
}

void Writer_t::proto_clearEvents()
{
}

void Writer_t::decl_stateList()
{
    out_h << getIndent(0) << "enum class " << reader.getModelName() << "_State" << std::endl;
    out_h << getIndent(0) << "{" << std::endl;
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (nullptr != state)
        {
            /* Only write down state on actual states that the machine may stay in. */
            if (("initial" != state->name) && ("final" != state->name) && (!state->isChoice))
            {
                out_h << getIndent(1) << styler.getStateNamePure(state) << "," << std::endl;
            }
        }
    }
    out_h << getIndent(0) << "};" << std::endl << std::endl;
}

void Writer_t::decl_eventList()
{
    const size_t nInEvents = reader.getInEventCount();
    const size_t nOutEvents = reader.getOutEventCount();
    const size_t nTimeEvents = reader.getTimeEventCount();
    const size_t nInternalEvents = reader.getInternalEventCount();

    // create an enum of all out-event names, out-events are on a separate queue since these are cleared by the user.
    if (0 < nOutEvents)
    {
        out_h << getIndent(0) << "enum class " << reader.getModelName() << "_OutEventId" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;
        for (std::size_t i = 0; i < nOutEvents; i++)
        {
            auto ev = reader.getOutEvent(i);
            if (nullptr != ev)
            {
                out_h << getIndent(1) << ev->name << "," << std::endl;
            }
        }
        out_h << getIndent(0) << "};" << std::endl << std::endl;

        // create a union of any event data possible.
        std::vector<std::pair<std::string, std::string>> paramData {};
        for (std::size_t i = 0; i < nOutEvents; i++)
        {
            auto ev = reader.getOutEvent(i);
            if ((nullptr != ev) && ev->requireParameter)
            {
                paramData.emplace_back(ev->parameterType, ev->name);
            }
        }
        if (!paramData.empty())
        {
            out_h << getIndent(0) << "union " << reader.getModelName() << "_OutEventData" << std::endl;
            out_h << getIndent(0) << "{" << std::endl;
            for (const auto& x : paramData)
            {
                out_h << getIndent(1) << x.first << " " << x.second << ";" << std::endl;
            }
            out_h << getIndent(1) << reader.getModelName() << "_OutEventData() = default;" << std::endl; //: ";
            out_h << getIndent(1) << "~" << reader.getModelName() << "_OutEventData() = default;" << std::endl;
            out_h << getIndent(0) << "};" << std::endl << std::endl;
        }

        // create struct containing the in-event.
        out_h << getIndent(0) << "struct " << reader.getModelName() << "_OutEvent" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;
        out_h << getIndent(1) << reader.getModelName() << "_OutEventId id;" << std::endl;
        if (!paramData.empty())
        {
            out_h << getIndent(1) << reader.getModelName() << "_OutEventData parameter;" << std::endl;
        }
        out_h << getIndent(1) << reader.getModelName() << "_OutEvent() = default;" << std::endl;
        out_h << getIndent(1) << "~" << reader.getModelName() << "_OutEvent() = default;" << std::endl;
        out_h << "};" << std::endl << std::endl;
    }

    if (0 < nTimeEvents)
    {
        out_h << getIndent(0) << "struct " << reader.getModelName() << "_TimeEvent" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;
        out_h << getIndent(1) << "bool isStarted;" << std::endl;
        out_h << getIndent(1) << "bool isPeriodic;" << std::endl;
        out_h << getIndent(1) << "size_t timeout_ms;" << std::endl;
        out_h << getIndent(1) << "size_t expireTime_ms;" << std::endl;
        out_h << getIndent(1) << reader.getModelName() << "_TimeEvent() = default;" << std::endl;
        out_h << getIndent(1) << "~" << reader.getModelName() << "_TimeEvent() = default;" << std::endl;
        out_h << getIndent(0) << "};" << std::endl << std::endl;

        out_h << getIndent(0) << "struct " << reader.getModelName() << "_TimeEvents" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;
        for (std::size_t i = 0; i < nTimeEvents; i++)
        {
            auto ev = reader.getTimeEvent(i);
            if (nullptr != ev)
            {
                out_h << getIndent(1) << reader.getModelName() << "_TimeEvent " << ev->name << ";" << std::endl;
            }
        }
        out_h << getIndent(0) << "};" << std::endl << std::endl;
    }

    // create an enum of all in-event names
    if ((0 < nInEvents) || (0 < nTimeEvents) || (0 < nInternalEvents))
    {
        out_h << getIndent(0) << "enum class " << reader.getModelName() << "_EventId" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;
        for (std::size_t i = 0; i < nInEvents; i++)
        {
            auto ev = reader.getInEvent(i);
            if (nullptr != ev)
            {
                out_h << getIndent(1) << "in_" << ev->name << "," << std::endl;
            }
        }
        for (std::size_t i = 0; i < nTimeEvents; i++)
        {
            auto ev = reader.getTimeEvent(i);
            if (nullptr != ev)
            {
                out_h << getIndent(1) << "time_" << ev->name << "," << std::endl;
            }
        }
        for (std::size_t i = 0; i < nInternalEvents; i++)
        {
            auto ev = reader.getInternalEvent(i);
            if (nullptr != ev)
            {
                out_h << getIndent(1) << "internal_" << ev->name << "," << std::endl;
            }
        }
        out_h << getIndent(0) << "};" << std::endl << std::endl;

        // create a union of any event data possible.
        std::vector<std::pair<std::string, std::string>> paramData {};
        for (std::size_t i = 0; i < nInEvents; i++)
        {
            auto ev = reader.getInEvent(i);
            if ((nullptr != ev) && ev->requireParameter)
            {
                paramData.emplace_back(ev->parameterType, "in_" + ev->name);
            }
        }
        for (std::size_t i = 0; i < nInternalEvents; i++)
        {
            auto ev = reader.getInternalEvent(i);
            if ((nullptr != ev) && ev->requireParameter)
            {
                paramData.emplace_back(ev->parameterType, "internal_" + ev->name);
            }
        }
        if (!paramData.empty())
        {
            out_h << getIndent(0) << "union " << reader.getModelName() << "_EventData" << std::endl;
            out_h << getIndent(0) << "{" << std::endl;
            for (const auto& x : paramData)
            {
                out_h << getIndent(1) << x.first << " " << x.second << ";" << std::endl;
            }
            out_h << getIndent(1) << reader.getModelName() << "_EventData() = default;" << std::endl;
            out_h << getIndent(1) << "~" << reader.getModelName() << "_EventData() = default;" << std::endl;
            out_h << getIndent(0) << "};" << std::endl << std::endl;
        }

        // create struct containing the in-event.
        out_h << getIndent(0) << "struct " << reader.getModelName() << "_Event" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;
        out_h << getIndent(1) << reader.getModelName() << "_EventId id;" << std::endl;
        if (!paramData.empty())
        {
            out_h << getIndent(1) << reader.getModelName() << "_EventData parameter;" << std::endl;
        }
        out_h << getIndent(1) << reader.getModelName() << "_Event() = default;" << std::endl;
        out_h << getIndent(1) << "~" << reader.getModelName() << "_Event() = default;" << std::endl;
        out_h << "};" << std::endl << std::endl;
    }
}

void Writer_t::decl_variableList()
{
    const size_t nPrivate = reader.getPrivateVariableCount();
    const size_t nPublic  = reader.getPublicVariableCount();

    if ((0 == nPrivate) && (0 == nPublic))
    {
        // no variables!
    }
    else
    {
        out_h << getIndent(0) << "struct " << reader.getModelName() << "_Variables" << std::endl;
        out_h << getIndent(0) << "{" << std::endl;

        // write private
        if (0 < nPrivate)
        {
            out_h << getIndent(1) << "struct " << reader.getModelName() << "_InternalVariables" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nPrivate; i++)
            {
                Variable_t* var = reader.getPrivateVariable(i);
                if ((nullptr != var) && (var->isPrivate))
                {
                    out_h << getIndent(2) << var->type << " " << var->name << ";" << std::endl;
                }
            }
            out_h << getIndent(2) << reader.getModelName() << "_InternalVariables() : ";
            for (size_t i = 0; i < nPrivate; i++)
            {
                Variable_t* var = reader.getPrivateVariable(i);
                if ((nullptr != var) && var->isPrivate)
                {
                    out_h << var->name << "()";
                    if (i < (nPrivate - 1))
                    {
                        out_h << ", ";
                    }
                }
            }
            out_h << " {}" << std::endl;
            out_h << getIndent(2) << "~" << reader.getModelName() << "_InternalVariables() = default;" << std::endl;
            out_h << getIndent(1) << "} internal;" << std::endl;
        }

        // write public
        if (0 < nPublic)
        {
            out_h << getIndent(1) << "struct " << reader.getModelName() << "_ExportedVariables" << std::endl;
            out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nPublic; i++)
            {
                Variable_t* var = reader.getPublicVariable(i);
                if ((nullptr != var) && !var->isPrivate)
                {
                    out_h << getIndent(2) << var->type << " " << var->name << ";" << std::endl;
                }
            }
            out_h << getIndent(2) << reader.getModelName() << "_ExportedVariables() : ";
            for (size_t i = 0; i < nPublic; i++)
            {
                Variable_t* var = reader.getPublicVariable(i);
                if ((nullptr != var) && !var->isPrivate)
                {
                    out_h << var->name << "()";
                    if (i < (nPublic - 1))
                    {
                        out_h << ", ";
                    }
                }
            }
            out_h << " {}" << std::endl;
            out_h << getIndent(2) << "~" << reader.getModelName() << "_ExportedVariables() = default;" << std::endl;
            out_h << getIndent(1) << "} exported;" << std::endl;
        }
        out_h << getIndent(1) << reader.getModelName() << "_Variables() : ";
        bool written = false;
        if (0 < nPrivate)
        {
            out_h << "internal()";
            written = true;
        }
        if (0 < nPublic)
        {
            if (written)
            {
                out_h << ", ";
            }
            out_h << "exported()";
            written = true;
        }
        out_h << " {}" << std::endl;
        out_h << getIndent(1) << "~" << reader.getModelName() << "_Variables() = default;" << std::endl;
        out_h << getIndent(0) << "};" << std::endl << std::endl;
    }
}

void Writer_t::decl_tracingCallback()
{
    out_h << getIndent(0) << "using TraceEntry_t = std::function<void(" << styler.getStateType() << " state)>;" << std::endl;
    out_h << getIndent(0) << "using TraceExit_t = std::function<void(" << styler.getStateType() << " state)>;" << std::endl << std::endl;
}

void Writer_t::decl_stateMachine()
{
    // write internal structure
    out_h << "///\\brief State machine base class for " << reader.getModelName() << "." << std::endl;
    out_h << getIndent(0) << "class " << reader.getModelName() << std::endl;
    out_h << getIndent(0) << "{" << std::endl;
    out_h << getIndent(0) << "private:" << std::endl;
    out_h << getIndent(1) << reader.getModelName() << "_State state;" << std::endl;
    if (0 < reader.getTimeEventCount())
    {
        out_h << getIndent(1) << reader.getModelName() << "_TimeEvents time_events;" << std::endl;
    }
    if ((0 < reader.getInEventCount()) || (0 < reader.getTimeEventCount()) || (0 < reader.getInternalEventCount()))
    {
        out_h << getIndent(1) << "std::deque<" << reader.getModelName() << "_Event> event_queue;" << std::endl;
    }
    if (0 < reader.getOutEventCount())
    {
        out_h << getIndent(1) << "std::deque<" << reader.getModelName() << "_OutEvent> out_event_queue;" << std::endl;
    }
    if (0 < reader.getVariableCount())
    {
        out_h << getIndent(1) << reader.getModelName() << "_Variables variables;" << std::endl;
    }
    if (config.doTracing)
    {
        out_h << getIndent(1) << "TraceEntry_t trace_enter_function;" << std::endl;
        out_h << getIndent(1) << "TraceExit_t trace_exit_function;" << std::endl;
    }
    if (0 < reader.getTimeEventCount())
    {
        // time now counter
        out_h << getIndent(1) << "size_t time_now_ms;" << std::endl;
    }
    out_h << getIndent(1) << "void " << styler.getTopRunCycle() << "();" << std::endl;
    if (config.doTracing)
    {
        out_h << getIndent(1) << "void " << styler.getTraceEntry() << "(" << styler.getStateType() << " state);" << std::endl;
        out_h << getIndent(1) << "void " << styler.getTraceExit() << "(" << styler.getStateType() << " state);" << std::endl;
    }
    for (size_t i = 0; i < reader.getInternalEventCount(); i++)
    {
        Event_t* ev = reader.getInternalEvent(i);
        out_h << getIndent(1) << "void " << styler.getEventRaise(ev) << "(";
        if (ev->requireParameter)
        {
            out_h << ev->parameterType << " value";
        }
        out_h << ");" << std::endl;
    }
    for (size_t i = 0; i < reader.getOutEventCount(); i++)
    {
        Event_t* ev = reader.getOutEvent(i);
        out_h << getIndent(1) << "void " << styler.getEventRaise(ev) << "(";
        if (ev->requireParameter)
        {
            out_h << ev->parameterType << " value";
        }
        out_h << ");" << std::endl;
    }
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (("initial" != state->name) && hasEntryStatement(state->id))
        {
            out_h << getIndent(1) << "void " << styler.getStateEntry(state) << "();" << std::endl;
        }
    }
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (("initial" != state->name) && hasExitStatement(state->id))
        {
            out_h << getIndent(1) << "void " << styler.getStateExit(state) << "();" << std::endl;
        }
    }
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (("initial" == state->name) || ("final" == state->name))
        {
            // no run cycle for initial or final states.
        }
        else if (state->isChoice)
        {
            // no run cycle for choices.
        }
        else
        {
            out_h << getIndent(1) << "bool " << styler.getStateRunCycle(state) << "(const " << reader.getModelName() << "_Event& event, bool try_transition);" << std::endl;
        }
    }
    out_h << std::endl;
    out_h << getIndent(0) << "public:" << std::endl;
    out_h << getIndent(1) << reader.getModelName() << "() : ";
    out_h << "state()";
    if (0 < reader.getTimeEventCount())
    {
        out_h << ", time_events()";
    }
    if ((0 < reader.getInEventCount()) || (0 < reader.getTimeEventCount()) || (0 < reader.getInternalEventCount()))
    {
        out_h << ", event_queue()";
    }
    if (0 < reader.getOutEventCount())
    {
        out_h << ", out_event_queue()";
    }
    if (0 < reader.getVariableCount())
    {
        out_h << ", variables()";
    }
    if (0 < reader.getTimeEventCount())
    {
        out_h << ", time_now_ms()";
    }
    out_h << " {}" << std::endl;
    out_h << getIndent(1) << "~" << reader.getModelName() << "() = default;" << std::endl;
    // add all prototypes.
    if (config.doTracing)
    {
        out_h << getIndent(1) << "void set_trace_enter_callback(const TraceEntry_t& enter_cb);" << std::endl;
        out_h << getIndent(1) << "void set_trace_exit_callback(const TraceExit_t& exit_cb);" << std::endl;
        out_h << getIndent(1) << "static std::string get_state_name(" << styler.getStateType() << " s);" << std::endl;
        out_h << getIndent(1) << "[[nodiscard]] " << styler.getStateType() << " get_state() const;" << std::endl;
    }
    out_h << getIndent(1) << "void init();" << std::endl;
    if (0 < reader.getTimeEventCount())
    {
        out_h << getIndent(1) << "void " << styler.getTimeTick() << "(" << "size_t time_elapsed_ms);" << std::endl;
    }
    for (size_t i = 0; i < reader.getInEventCount(); i++)
    {
        Event_t* ev = reader.getInEvent(i);
        out_h << getIndent(1) << "void " << styler.getEventRaise(ev) << "(";
        if (ev->requireParameter)
        {
            out_h << ev->parameterType << " value";
        }
        out_h << ");" << std::endl;
    }
    if (0 < reader.getOutEventCount())
    {
        out_h << getIndent(1) << "bool is_out_event_raised(" << reader.getModelName() << "_OutEvent& ev);" << std::endl;
    }
#if 0
    for (size_t i = 0; i < reader.getOutEventCount(); i++)
    {
        Event_t* ev = reader.getOutEvent(i);
        out_h << getIndent(1) << "[[nodiscard]] bool " << styler.getEventIsRaised(ev) << "() const;" << std::endl;
        if (ev->requireParameter)
        {
            out_h << getIndent(1) << "[[nodiscard]] " << ev->parameterType << " " << styler.getEventValue(ev) << "() const;" << std::endl;
        }
    }
#endif
    for (size_t i = 0; i < reader.getVariableCount(); i++)
    {
        Variable_t* var = reader.getPublicVariable(i);
        if (nullptr != var)
        {
            out_h << getIndent(1) << "[[nodiscard]] " << var->type << " " << styler.getVariable(var) << "() const;" << std::endl;
        }
    }
    out_h << getIndent(0) << "};" << std::endl << std::endl;
}

void Writer_t::impl_init(const std::vector<State_t*>& firstState)
{
    out_c << "void " << reader.getModelName() << "::init()" << std::endl;
    out_c << "{" << std::endl;

    // write variable inits
    out_c << getIndent(1) << "// Initialise variables." << std::endl;
    bool any_specific_inited = false;
    for (size_t i = 0; i < reader.getVariableCount(); i++)
    {
        Variable_t* var = reader.getVariable(i);
        if (var->specificInitialValue)
        {
            if (var->isPrivate)
            {
                out_c << getIndent(1) << "variables.internal.";
            } else
            {
                out_c << getIndent(1) << "variables.exported.";
            }
            out_c << var->name << " = " << var->initialValue << ";" << std::endl;
            any_specific_inited = true;
        }
    }
    if (!any_specific_inited)
    {
        out_c << getIndent(1) << "// No variables with specific values defined, all initialised to 0." << std::endl;
    }
    out_c << std::endl;

    // enter first state
    if (!firstState.empty())
    {
        out_c << getIndent(1) << "// Set initial state." << std::endl;
        State_t* targetState = nullptr;
        for (size_t i = 0; i < firstState.size(); i++)
        {
            targetState = firstState[i];
            if (hasEntryStatement(targetState->id))
            {
                // write entry call
                out_c << getIndent(1) << styler.getStateEntry(targetState) << "();" << std::endl;
            }
        }
        out_c << getIndent(1) << "state = " << styler.getStateName(targetState) << ";" << std::endl;
        if (config.doTracing)
        {
            out_c << getIndent(1) << getTraceCall_entry(targetState) << std::endl;
        }
    }
    out_c << "}" << std::endl << std::endl;
}

void Writer_t::impl_raiseInEvent()
{
    for (size_t i = 0; i < reader.getInEventCount(); i++)
    {
        Event_t* ev = reader.getInEvent(i);
        out_c << getIndent(0) << "void " << reader.getModelName() << "::" << styler.getEventRaise(ev) << "(";
        if (ev->requireParameter)
        {
            out_c << ev->parameterType << " value";
        }
        out_c << ")" << std::endl;
        out_c << getIndent(0) << "{" << std::endl;
        out_c << getIndent(1) << reader.getModelName() << "_Event event {};" << std::endl;
        out_c << getIndent(1) << "event.id = " << reader.getModelName() << "_EventId::in_" << ev->name << ";" << std::endl;

        if (ev->requireParameter)
        {
            out_c << getIndent(1) << "event.parameter.in_" << ev->name << " = value;" << std::endl;
        }
        out_c << getIndent(1) << "event_queue.push_back(event);" << std::endl;
        out_c << getIndent(1) << styler.getTopRunCycle() << "();" << std::endl;
        out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_raiseOutEvent()
{
    for (size_t i = 0; i < reader.getOutEventCount(); i++)
    {
        Event_t* ev = reader.getOutEvent(i);
        out_c << getIndent(0) << "void " << reader.getModelName() << "::" << styler.getEventRaise(ev) << "(";
        if (ev->requireParameter)
        {
            out_c << ev->parameterType << " value";
        }
        out_c << ")" << std::endl;
        out_c << getIndent(0) << "{" << std::endl;
        out_c << getIndent(1) << reader.getModelName() << "_OutEvent event {};" << std::endl;
        out_c << getIndent(1) << "event.id = " << reader.getModelName() << "_OutEventId::" << ev->name << ";" << std::endl;
        if (ev->requireParameter)
        {
            out_c << getIndent(1) << "event.parameter." << ev->name << " = value;" << std::endl;
        }
        out_c << getIndent(1) << "out_event_queue.push_back(event);" << std::endl;
        out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_raiseInternalEvent()
{
    for (size_t i = 0; i < reader.getInternalEventCount(); i++)
    {
        Event_t* ev = reader.getInternalEvent(i);
        out_c << getIndent(0) << "void " << reader.getModelName() << "::" << styler.getEventRaise(ev) << "(";
        if (ev->requireParameter)
        {
            out_c << ev->parameterType << " value";
        }
        out_c << ")" << std::endl;
        out_c << getIndent(0) << "{" << std::endl;
        out_c << getIndent(1) << reader.getModelName() << "_Event event {};" << std::endl;
        out_c << getIndent(1) << "event.id = " << reader.getModelName() << "_EventId::internal_" << ev->name << ";" << std::endl;
        if (ev->requireParameter)
        {
            out_c << getIndent(1) << "event.parameter.internal_" << ev->name << " = value;" << std::endl;
        }
        out_c << getIndent(1) << "event_queue.push_back(event);" << std::endl;
        out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_checkOutEvent()
{
    if (0 < reader.getOutEventCount())
    {
        out_c << getIndent(0) << "bool " << reader.getModelName() << "::is_out_event_raised(" << reader.getModelName() << "_OutEvent& ev)" << std::endl;
        out_c << getIndent(0) << "{" << std::endl;
        out_c << getIndent(1) << "bool pending = false;" << std::endl;
        out_c << getIndent(1) << "if (!out_event_queue.empty())" << std::endl;
        out_c << getIndent(1) << "{" << std::endl;
        out_c << getIndent(2) << "ev = out_event_queue.front();" << std::endl;
        out_c << getIndent(2) << "out_event_queue.pop_front();" << std::endl;
        out_c << getIndent(2) << "pending = true;" << std::endl;
        out_c << getIndent(1) << "}" << std::endl;
        out_c << getIndent(1) << "return pending;" << std::endl;
        out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
#if 0
    for (size_t i = 0; i < reader.getOutEventCount(); i++)
    {
        Event_t* ev = reader.getOutEvent(i);
        out_c << "bool " << reader.getModelName() << "::" << styler.getEventIsRaised(ev) << "() const" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "return events.outEvents." << ev->name << "_isRaised;" << std::endl;
        out_c << "}" << std::endl << std::endl;
    }
#endif
}

void Writer_t::impl_getVariable()
{
    for (size_t i = 0; i < reader.getVariableCount(); i++)
    {
        Variable_t* var = reader.getPublicVariable(i);
        if (nullptr != var)
        {
            out_c << var->type << " " << reader.getModelName() << "::" << styler.getVariable(var) << "() const" << std::endl;
            out_c << "{" << std::endl;
            out_c << getIndent(1) << "return variables.exported." << var->name << ";" << std::endl;
            out_c << "}" << std::endl << std::endl;
        }
    }
}

void Writer_t::impl_timeTick()
{
    if (0 < reader.getTimeEventCount())
    {
        out_c << getIndent(0) << "void " << reader.getModelName() << "::" << styler.getTimeTick() << "(size_t time_elapsed_ms)" << std::endl;
        out_c << getIndent(0) << "{" << std::endl;
        out_c << getIndent(1) << "time_now_ms += time_elapsed_ms;" << std::endl << std::endl;
        for (size_t i = 0; i < reader.getTimeEventCount(); i++)
        {
            Event_t* ev = reader.getTimeEvent(i);
            if (nullptr != ev)
            {
                out_c << getIndent(1) << "if (time_events." << ev->name << ".isStarted)" << std::endl;
                out_c << getIndent(1) << "{" << std::endl;
                out_c << getIndent(2) << "if (time_events." << ev->name << ".expireTime_ms <= time_now_ms)" << std::endl;
                out_c << getIndent(2) << "{" << std::endl;
                out_c << getIndent(3) << "// Time events does not carry any parameter." << std::endl;
                out_c << getIndent(3) << reader.getModelName() << "_Event event {};" << std::endl;
                out_c << getIndent(3) << "event.id = " << reader.getModelName() << "_EventId::time_" << ev->name << ";" << std::endl;
                out_c << getIndent(3) << "event_queue.push_back(event);" << std::endl << std::endl;
                //out_c << getIndent(3) << "time_events." << ev->name << ".isRaised = true;" << std::endl;
                out_c << getIndent(3) << "// Check for automatic reload." << std::endl;
                out_c << getIndent(3) << "if (time_events." << ev->name << ".isPeriodic)" << std::endl;
                out_c << getIndent(3) << "{" << std::endl;
                out_c << getIndent(4) << "time_events." << ev->name << ".expireTime_ms += time_events." << ev->name << ".timeout_ms;" << std::endl;
                out_c << getIndent(4) << "time_events." << ev->name << ".isStarted = true;" << std::endl;
                out_c << getIndent(3) << "}" << std::endl;
                out_c << getIndent(3) << "else" << std::endl;
                out_c << getIndent(3) << "{" << std::endl;
                out_c << getIndent(4) << "time_events." << ev->name << ".isStarted = false;" << std::endl;
                out_c << getIndent(3) << "}" << std::endl;
                out_c << getIndent(2) << "}" << std::endl;
                out_c << getIndent(1) << "}" << std::endl;
            }
        }
        out_c << getIndent(1) << styler.getTopRunCycle() << "();" << std::endl;
        out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_clearEvents()
{
#if 0
    if (0 < reader.getInEventCount())
    {
        out_c << "void " << reader.getModelName() << "::clear_in_events()" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "events.inEvents = {};" << std::endl;
        out_c << "}" << std::endl << std::endl;
    }

    if (0 < reader.getInternalEventCount())
    {
        out_c << "void " << reader.getModelName() << "::clear_internal_events()" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "events.internalEvents = {};" << std::endl;
        out_c << "}" << std::endl << std::endl;
    }

    if (0 < reader.getTimeEventCount())
    {
        out_c << "void " << reader.getModelName() << "::clear_time_events()" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "events.timeEvents = {};" << std::endl;
        out_c << "}" << std::endl << std::endl;
    }

    if (0 < reader.getOutEventCount())
    {
        out_c << "void " << reader.getModelName() << "::clear_out_events()" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "events.outEvents = {};" << std::endl;
        out_c << "}" << std::endl << std::endl;
    }
#endif
}

void Writer_t::impl_checkAnyEventRaised()
{
#if 0
    if (0 < reader.getInternalEventCount())
    {
        out_c << "bool " << reader.getModelName() << "::is_internal_event_raised()" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "bool any_raised = false;" << std::endl;
        for (std::size_t i = 0; i < reader.getInternalEventCount(); i++)
        {
            auto ev = reader.getInternalEvent(i);
            if (nullptr != ev)
            {
                out_c << getIndent(1) << "any_raised |= events.internalEvents." << ev->name << "_isRaised;" << std::endl;
            }
        }
        out_c << getIndent(1) << "return any_raised;" << std::endl;
        out_c << "}" << std::endl;
    }
#endif
}

void Writer_t::impl_topRunCycle()
{
    size_t writeNumber = 0;
    size_t indent = 0;
    out_c << "void " << reader.getModelName() << "::" << styler.getTopRunCycle() << "()" << std::endl;
    out_c << "{" << std::endl;
    indent++;

#if 0
    if (0 < reader.getOutEventCount())
    {
        out_c << getIndent(indent) << "clear_out_events();" << std::endl << std::endl;
    }
#endif

    out_c << getIndent(indent) << "// Handle all queued events." << std::endl;
    out_c << getIndent(indent) << "while (!event_queue.empty())" << std::endl;
    out_c << getIndent(indent) << "{" << std::endl;
    indent++;

    out_c << getIndent(indent) << "auto event = event_queue.front();" << std::endl;
    out_c << getIndent(indent) << "event_queue.pop_front();" << std::endl << std::endl;

    out_c << getIndent(indent) << "switch (state)" << std::endl;
    out_c << getIndent(indent) << "{" << std::endl;
    indent++;
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (("initial" == state->name) || ("final" == state->name))
        {
            // no handling on initial or final states.
        }
        else if (state->isChoice)
        {
            // no handling on choice.
        }
        else
        {
            out_c << getIndent(indent) << "case " << styler.getStateName(state) << ":" << std::endl;
            indent++;
            out_c << getIndent(indent) << styler.getStateRunCycle(state) << "(event, true);" << std::endl;
            out_c << getIndent(indent) << "break;" << std::endl << std::endl;
            indent--;
#if 0
            out_c << getIndent(indent) << getIfElseIf(writeNumber) << " (" << styler.getStateName(state) << " == state)" << std::endl;
            out_c << getIndent(indent) << "{" << std::endl;
            indent++;
            out_c << getIndent(indent) << styler.getStateRunCycle(state) << "(true);" << std::endl;
            indent--;
            out_c << getIndent(indent) << "}" << std::endl;
            writeNumber++;
#endif
        }
    }
    if (0 < reader.getStateCount())
    {
        out_c << getIndent(indent) << "default:" << std::endl;
        indent++;
        out_c << getIndent(indent) << "// Invalid, or final state." << std::endl;
        out_c << getIndent(indent) << "break;" << std::endl;
        indent--;
#if 0
        out_c << getIndent(indent) << "else" << std::endl;
        out_c << getIndent(indent) << "{" << std::endl;
        indent++;
        out_c << getIndent(indent) << "// possibly final state!" << std::endl;
        indent--;
        out_c << getIndent(indent) << "}" << std::endl;
#endif
    }
    indent--;
    out_c << getIndent(indent) << "}" << std::endl;
    indent--;
    out_c << getIndent(indent) << "}" << std::endl;

#if 0
    if (0 < reader.getInEventCount())
    {
        out_c << getIndent(indent) << "clear_in_events();" << std::endl;
    }
    if (0 < reader.getTimeEventCount())
    {
        out_c << getIndent(indent) << "clear_time_events();" << std::endl;
    }
    if (0 < reader.getInternalEventCount())
    {
        out_c << getIndent(indent) << "clear_internal_events();" << std::endl;
    }
#endif
    out_c << "}" << std::endl << std::endl;
}

void Writer_t::impl_traceCalls()
{
    if (config.doTracing)
    {
        out_c << "void " << reader.getModelName() << "::" << styler.getTraceEntry() << "(" << styler.getStateType() << " entered_state)" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "if (nullptr != trace_enter_function)" << std::endl;
        out_c << getIndent(1) << "{" << std::endl;
        out_c << getIndent(2) << "trace_enter_function(entered_state);" << std::endl;
        out_c << getIndent(1) << "}" << std::endl;
        out_c << "}" << std::endl << std::endl;

        out_c << "void " << reader.getModelName() << "::" << styler.getTraceExit() << "(" << styler.getStateType() << " exited_state)" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "if (nullptr != trace_exit_function)" << std::endl;
        out_c << getIndent(1) << "{" << std::endl;
        out_c << getIndent(2) << "trace_exit_function(exited_state);" << std::endl;
        out_c << getIndent(1) << "}" << std::endl;
        out_c << "}" << std::endl << std::endl;

        out_c << "void " << reader.getModelName() << "::set_trace_enter_callback(const TraceEntry_t& enter_cb)" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "trace_enter_function = enter_cb;" << std::endl;
        out_c << "}" << std::endl << std::endl;

        out_c << "void " << reader.getModelName() << "::set_trace_exit_callback(const TraceExit_t& exit_cb)" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "trace_exit_function = exit_cb;" << std::endl;
        out_c << "}" << std::endl << std::endl;

        out_c << "std::string " << reader.getModelName() << "::get_state_name(" << styler.getStateType() << " s)" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "switch (s)" << std::endl;
        out_c << getIndent(1) << "{" << std::endl;
        for (std::size_t i = 0; i < reader.getStateCount(); i++)
        {
            auto s = reader.getState(i);
            /* Only write down state on actual states that the machine may stay in. */
            if ((nullptr != s) && ("initial" != s->name) && ("final" != s->name) && (!s->isChoice))
            {
                out_c << getIndent(2) << "case " << styler.getStateType() << "::" << styler.getStateNamePure(s) << ":" << std::endl;
                out_c << getIndent(3) << "return \"" << styler.getStateNamePure(s) << "\";" << std::endl << std::endl;
            }
        }
        out_c << getIndent(2) << "default:" << std::endl;
        out_c << getIndent(3) << "// Invalid state." << std::endl;
        out_c << getIndent(3) << "return {};" << std::endl;
        out_c << getIndent(1) << "}" << std::endl;
        out_c << "}" << std::endl << std::endl;

        out_c << styler.getStateType() << " " << reader.getModelName() << "::get_state() const" << std::endl;
        out_c << "{" << std::endl;
        out_c << getIndent(1) << "return state;" << std::endl;
        out_c << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_runCycle()
{
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (("initial" == state->name) || ("final" == state->name))
        {
            // initial or final state, no runcycle
        }
        else if (state->isChoice)
        {
            // no run cycle for choice
        }
        else
        {
            size_t indentLevel = 0;
            bool isEmptyBody = true;

            out_c << "bool " << reader.getModelName() << "::" << styler.getStateRunCycle(state) << "(const " << reader.getModelName() << "_Event& event, " << "bool try_transition)" << std::endl;
            out_c << "{" << std::endl;
            indentLevel++;

            // write comment declaration here if one exists.
            const size_t numCommentLines = reader.getDeclCount(state->id, Declaration_Comment);
            if (0 < numCommentLines)
            {
                isEmptyBody = false;
                for (size_t j = 0; j < numCommentLines; j++)
                {
                    State_Declaration_t* decl = reader.getDeclFromStateId(state->id, Declaration_Comment, j);
                    out_c << getIndent(indentLevel) << "// " << decl->declaration << std::endl;
                }
                out_c << std::endl;
            }

            out_c << getIndent(indentLevel) << "auto did_transition = try_transition;" << std::endl;
            out_c << getIndent(indentLevel) << "if (try_transition)" << std::endl;
            out_c << getIndent(indentLevel) << "{" << std::endl;
            indentLevel++;

            const size_t nOutTr = reader.getTransitionCountFromStateId(state->id);

            // write parent react
            State_t* parentState = reader.getStateById(state->parent);
            if (nullptr != parentState)
            {
                isEmptyBody = false;
                {
                    out_c << getIndent(indentLevel) << "if (!" << styler.getStateRunCycle(parentState) << "(event, try_transition))" << std::endl;
                    out_c << getIndent(indentLevel) << "{" << std::endl;
                    indentLevel++;
                }
            }

            if (0 == nOutTr)
            {
                out_c << getIndent(indentLevel) << "did_transition = false;" << std::endl;
            }
            else
            {
                for (size_t j = 0; j < nOutTr; j++)
                {
                    Transition_t* tr = reader.getTransitionFrom(state->id, j);
                    if ("null" == tr->event.name)
                    {
                        State_t* trStB = reader.getStateById(tr->stB);
                        if (nullptr == trStB)
                        {
                            errorReport("Null transition!", __LINE__);
                        }
                        else if ("final" != trStB->name)
                        {
                            errorReport("Null transition!", __LINE__);

                            // handle as a oncycle transition?
                            isEmptyBody = false;

                            out_c << getIndent(indentLevel) << getIfElseIf(j) << " (true)" << std::endl;
                            out_c << getIndent(indentLevel) << "{" << std::endl;
                            indentLevel++;

                            // if tracing is enabled
#if 0
                            if (writerConfig.doTracing)
                            {
                                writer->out_c << getIndent(indentLevel) << getTraceCall_exit(reader) << std::endl;
                            }
#endif

                            // is exit function exists
                            if (hasExitStatement(state->id))
                            {
                                out_c << getIndent(indentLevel) << styler.getStateExit(state) << "();" << std::endl;
                            }

                            indentLevel--;
                            out_c << getIndent(indentLevel) << "}" << std::endl;
                        }
                    }
                    else
                    {
                        State_t* trStB = reader.getStateById(tr->stB);
                        if (nullptr == trStB)
                        {
                            errorReport("Null transition!", __LINE__);
                        }
                        else
                        {
                            isEmptyBody = false;

                            if (tr->event.isTimeEvent)
                            {
                                if (tr->hasGuard)
                                {
                                    std::string guardStr = parseGuard(tr->guard);
                                    out_c << getIndent(indentLevel)
                                                  << getIfElseIf(j)
                                                  << " ((" << reader.getModelName() << "_EventId::time_"
                                                  << tr->event.name
                                                  << " == event.id) && ("
                                                  << guardStr
                                                  << "))"
                                                  << std::endl;
                                }
                                else
                                {
                                    out_c << getIndent(indentLevel)
                                                  << getIfElseIf(j)
                                                  << " (" << reader.getModelName() << "_EventId::time_"
                                                  << tr->event.name
                                                  << " == event.id)"
                                                  << std::endl;
                                }
                            }
                            else
                            {
                                if (tr->hasGuard)
                                {
                                    std::string guardStr = parseGuard(tr->guard);
                                    out_c << getIndent(indentLevel)
                                                  << getIfElseIf(j);
                                    if (Event_Direction_Incoming == tr->event.direction)
                                    {
                                        out_c << " ((" << reader.getModelName() << "_EventId::in_"
                                                      << tr->event.name
                                                      << " == event.id) && (";
                                    }
                                    else if (Event_Direction_Internal == tr->event.direction)
                                    {
                                        out_c << " ((" << reader.getModelName() << "_EventId::internal_"
                                                      << tr->event.name
                                                      << " == event.id) && (";
                                    }
                                    else
                                    {
                                        out_c << " ((" << reader.getModelName() << "_EventId::out_"
                                                      << tr->event.name
                                                      << " == event.id) && (";
                                    }
                                    out_c << guardStr
                                                  << "))"
                                                  << std::endl;
                                }
                                else
                                {
                                    if (Event_Direction_Incoming == tr->event.direction)
                                    {
                                        out_c << getIndent(indentLevel) << getIfElseIf(j)
                                            << " (" << reader.getModelName() << "_EventId::in_"
                                            << tr->event.name
                                            << " == event.id)" << std::endl;
                                    }
                                    else if (Event_Direction_Internal == tr->event.direction)
                                    {
                                        out_c << getIndent(indentLevel) << getIfElseIf(j)
                                            << " (" << reader.getModelName() << "_EventId::internal_"
                                            << tr->event.name
                                            << " == event.id)" << std::endl;
                                    }
                                    else
                                    {
                                        out_c << getIndent(indentLevel) << getIfElseIf(j)
                                            << " (" << reader.getModelName() << "_EventId::out_"
                                            << tr->event.name
                                            << " == event.id)" << std::endl;
                                    }
                                }
                            }
                            out_c << getIndent(indentLevel) << "{" << std::endl;
                            indentLevel++;

                            const bool didChildExits = parseChildExits(state, indentLevel, state->id, false);

                            if (didChildExits)
                            {
                                out_c << std::endl;
                            }
                            else
                            {
                                if (hasExitStatement(state->id))
                                {
                                    out_c << getIndent(indentLevel) << "// Handle super-step exit." << std::endl;
                                    out_c << getIndent(indentLevel) << styler.getStateExit(state) << "();" << std::endl;
                                }
                                if (config.doTracing)
                                {
                                    out_c << getIndent(indentLevel) << getTraceCall_exit(state) << std::endl;
                                }
                                /* Extra new-line */
                                if ((hasExitStatement(state->id)) || (config.doTracing))
                                {
                                    out_c << std::endl;
                                }
                            }

                            // TODO: do entry actins on all states entered
                            // towards the goal! Might needs some work..
                            std::vector<State_t*> enteredStates = findEntryState(trStB);

                            if (!enteredStates.empty())
                            {
                                out_c << getIndent(indentLevel) << "// Handle super-step entry." << std::endl;
                            }

                            State_t* finalState = nullptr;
                            for (size_t q = 0; q < enteredStates.size(); q++)
                            {
                                finalState = enteredStates[q];

                                if (hasEntryStatement(finalState->id))
                                {
                                    out_c << getIndent(indentLevel) << styler.getStateEntry(finalState) << "();" << std::endl;
                                }

                                if (config.doTracing)
                                {
                                    // Don't trace entering the choice states, since the state does not exist.
                                    if (!finalState->isChoice)
                                    {
                                        out_c << getIndent(indentLevel) << getTraceCall_entry(finalState) << std::endl;
                                    }
                                }
                            }

                            // handle choice node?
                            if ((nullptr != finalState) && (finalState->isChoice))
                            {
                                parseChoicePath(finalState, indentLevel);
                            }
                            else
                            {
                                out_c << getIndent(indentLevel) << "state = " << styler.getStateName(finalState) << ";" << std::endl;
                            }

                            indentLevel--;
                            out_c << getIndent(indentLevel) << "}" << std::endl;
                        }
                    }
                }

                out_c << getIndent(indentLevel) << "else" << std::endl;
                out_c << getIndent(indentLevel) << "{" << std::endl;
                indentLevel++;
                out_c << getIndent(indentLevel) << "did_transition = false;" << std::endl;
                indentLevel--;
                out_c << getIndent(indentLevel) << "}" << std::endl;
            }

            while (1 < indentLevel)
            {
                indentLevel--;
                out_c << getIndent(indentLevel) << "}" << std::endl;
            }

#if 0
            if (isEmptyBody)
            {
                out_c << getIndent(indentLevel) << "(void) try_transition;" << std::endl;
            }
#endif

            out_c << getIndent(indentLevel) << "return did_transition;" << std::endl;
            out_c << "}" << std::endl << std::endl;
        }
    }
}

void Writer_t::impl_entryAction()
{
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader.getDeclCount(state->id, Declaration_Entry);
            size_t numTimeEv = 0;
            for (size_t j = 0; j < reader.getTransitionCountFromStateId(state->id); j++)
            {
                Transition_t* tr = reader.getTransitionFrom(state->id, j);
                if ((nullptr != tr) && (tr->event.isTimeEvent))
                {
                    numTimeEv++;
                }
            }

            if ((0 < numDecl) || (0 < numTimeEv))
            {
                out_c << getIndent(0) << "void " << reader.getModelName() << "::" << styler.getStateEntry(state) << "()" << std::endl;
                out_c << getIndent(0) << "{" << std::endl;

                // start timers
                size_t writeIndex = 0;
                for (size_t j = 0; j < reader.getTransitionCountFromStateId(state->id); j++)
                {
                    Transition_t* tr = reader.getTransitionFrom(state->id, j);
                    if ((nullptr != tr) && (tr->event.isTimeEvent))
                    {
                        out_c << getIndent(1) << "/* Start timer " << tr->event.name << " with timeout of " << tr->event.expireTime_ms << " ms. */" << std::endl;
                        out_c << getIndent(1) << "time_events." << tr->event.name << ".timeout_ms = " << tr->event.expireTime_ms << ";" << std::endl;
                        out_c << getIndent(1) << "time_events." << tr->event.name << ".expireTime_ms = time_now_ms + " << tr->event.expireTime_ms << ";" << std::endl;
                        out_c << getIndent(1) << "time_events." << tr->event.name << ".isPeriodic = " << (tr->event.isPeriodic ? "true;" : "false;") << std::endl;
                        out_c << getIndent(1) << "time_events." << tr->event.name << ".isStarted = true;" << std::endl;
                        writeIndex++;
                        if (writeIndex < numTimeEv)
                        {
                            out_c << std::endl;
                        }
                    }
                }

                if ((0 < numDecl) && (0 < numTimeEv))
                {
                    // add a space between the parts
                    out_c << std::endl;
                }

                for (size_t j = 0; j < numDecl; j++)
                {
                    State_Declaration_t* decl = reader.getDeclFromStateId(state->id, Declaration_Entry, j);
                    if (Declaration_Entry == decl->type)
                    {
                        parseDeclaration(decl->declaration);
                    }
                }
                out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
}

void Writer_t::impl_exitAction()
{
    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader.getDeclCount(state->id, Declaration_Exit);
            size_t numTimeEv = 0;
            for (size_t j = 0; j < reader.getTransitionCountFromStateId(state->id); j++)
            {
                Transition_t* tr = reader.getTransitionFrom(state->id, j);
                if ((nullptr != tr) && (tr->event.isTimeEvent))
                {
                    numTimeEv++;
                }
            }

            if ((0 < numDecl) || (0 < numTimeEv))
            {
                out_c << getIndent(0) << "void " << reader.getModelName() << "::" << styler.getStateExit(state) << "()" << std::endl;
                out_c << getIndent(0) << "{" << std::endl;

                // stop timers
                for (size_t j = 0; j < reader.getTransitionCountFromStateId(state->id); j++)
                {
                    Transition_t* tr = reader.getTransitionFrom(state->id, j);
                    if ((nullptr != tr) && (tr->event.isTimeEvent))
                    {
                        out_c << getIndent(1) << "time_events." << tr->event.name << ".isStarted = false;" << std::endl;
                    }
                }

                if ((0 < numDecl) && (0 < numTimeEv))
                {
                    // add a space between the parts
                    out_c << std::endl;
                }

                if (0 < numDecl)
                {
                    for (size_t j = 0; j < numDecl; j++)
                    {
                        State_Declaration_t* decl = reader.getDeclFromStateId(state->id, Declaration_Exit, j);
                        if (Declaration_Exit == decl->type)
                        {
                            parseDeclaration(decl->declaration);
                        }
                    }
                }
                out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
}

std::vector<std::string> Writer_t::tokenize(std::string str)
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

void Writer_t::parseDeclaration(const std::string& declaration)
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

                for (size_t i = 0; i < reader.getVariableCount(); i++)
                {
                    Variable_t* var = reader.getVariable(i);
                    if (0 == var->name.compare(replaceString))
                    {
                        wstr += "variables.";
                        if (var->isPrivate)
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
                    for (size_t i = 0; i < reader.getInEventCount(); i++)
                    {
                        Event_t* ev = reader.getInEvent(i);
                        if (0 == ev->name.compare(replaceString))
                        {
                            wstr += "events.inEvents." + ev->name + "_value";
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
                        outstr += styler.getEventRaise(tokens[0]) + "(";
                        if (ev->requireParameter)
                        {
                            if (tokens.size() < 2)
                            {
                                outstr += "{}";
                            } else
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

    out_c << getIndent(1) << outstr << std::endl;
}

std::string Writer_t::parseGuard(const std::string& guardStrRaw)
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

                for (size_t i = 0; i < reader.getVariableCount(); i++)
                {
                    Variable_t* var = reader.getVariable(i);
                    if (0 == var->name.compare(replaceString))
                    {
                        wstr += "variables.";
                        if (var->isPrivate)
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
                    for (size_t i = 0; i < reader.getInEventCount(); i++)
                    {
                        Event_t* ev = reader.getInEvent(i);
                        if (0 == ev->name.compare(replaceString))
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

void Writer_t::parseChoicePath(State_t* state, size_t indentLevel)
{
    // check all transitions from the choice..
    out_c << std::endl << getIndent(indentLevel) << "/* Choice: " << state->name << " */" << std::endl;

    const size_t numChoiceTr = reader.getTransitionCountFromStateId(state->id);
    if (numChoiceTr < 2)
    {
        errorReport("Ony one transition from choice " + state->name, __LINE__);
    }
    else
    {
        Transition_t* defaultTr = nullptr;
        size_t k = 0;
        for (size_t j = 0; j < numChoiceTr; j++)
        {
            Transition_t* tr = reader.getTransitionFrom(state->id, j);
            if (true != tr->hasGuard)
            {
                defaultTr = tr;
            }
            else
            {
                // handle if statement
                out_c << getIndent(indentLevel + 0) << getIfElseIf(k++) << " (" << parseGuard(tr->guard) << ")" << std::endl;
                out_c << getIndent(indentLevel + 0) << "{" << std::endl;
                State_t* guardedState = reader.getStateById(tr->stB);
                out_c << getIndent(indentLevel + 1) << "// goto: " << guardedState->name << std::endl;
                if (nullptr == guardedState)
                {
                    errorReport("Invalid transition from choice " + state->name, __LINE__);
                }
                else
                {
                    std::vector<State_t*> enteredStates = findEntryState(guardedState);
                    State_t* finalState = nullptr;
                    for (size_t j = 0; j < enteredStates.size(); j++)
                    {
                        finalState = enteredStates[j];

                        // TODO: fix call so send state name.
#if 0
                        if (writerConfig.doTracing)
                        {
                            out_c << getIndent(indentLevel + 1) << getTraceCall_entry(reader) << std::endl;
                        }
#endif

                        if (0 < reader.getDeclCount(finalState->id, Declaration_Entry))
                        {
                            out_c << getIndent(indentLevel + 1) << styler.getStateEntry(finalState) << "();" << std::endl;
                        }
                    }
                    if (nullptr != finalState)
                    {
                        if (finalState->isChoice)
                        {
                            // nest ..
                            parseChoicePath(finalState, indentLevel + 1);
                        }
                        else
                        {
                            out_c << getIndent(indentLevel + 1) << "state = " << styler.getStateName(finalState) << ";" << std::endl;
                        }
                    }
                }
                out_c << getIndent(indentLevel + 0) << "}" << std::endl;
            }
        }
        if (nullptr != defaultTr)
        {
            // write default transition.
            out_c << getIndent(indentLevel + 0) << "else" << std::endl;
            out_c << getIndent(indentLevel + 0) << "{" << std::endl;
            State_t* guardedState = reader.getStateById(defaultTr->stB);
            out_c << getIndent(indentLevel + 1) << "// goto: " << guardedState->name << std::endl;
            if (nullptr == guardedState)
            {
                errorReport("Invalid transition from choice " + state->name, __LINE__);
            }
            else
            {
                std::vector<State_t*> enteredStates = findEntryState(guardedState);
                State_t* finalState = nullptr;
                for (size_t j = 0; j < enteredStates.size(); j++)
                {
                    finalState = enteredStates[j];

                    // TODO: fix call so send state name.
#if 0
                    if (writerConfig.doTracing)
                    {
                        writer->out_c << getIndent(indentLevel + 1) << getTraceCall_entry(reader) << std::endl;
                    }
#endif

                    if (0 < reader.getDeclCount(finalState->id, Declaration_Entry))
                    {
                        out_c << getIndent(indentLevel + 1) << styler.getStateEntry(finalState) << "();" << std::endl;
                    }
                }
                if (nullptr != finalState)
                {
                    if (finalState->isChoice)
                    {
                        // nest ..
                        parseChoicePath(finalState, indentLevel + 1);
                    }
                    else
                    {
                        out_c << getIndent(indentLevel + 1) << "state = " << styler.getStateName(finalState) << ";" << std::endl;
                    }
                }
            }
            out_c << getIndent(indentLevel + 0) << "}" << std::endl;
        }
        else
        {
            errorReport("No default transition from " + state->name, __LINE__);
        }
    }
}

std::vector<State_t*> Writer_t::getChildStates(State_t* currentState)
{
    std::vector<State_t*> childStates;
    for (size_t j = 0; j < reader.getStateCount(); j++)
    {
        State_t* child = reader.getState(j);

        if ((nullptr != child) &&
            (child->parent == currentState->id) &&
            (0 != child->name.compare("initial")) &&
            (0 != child->name.compare("final")) &&
            (!child->isChoice))
        {
            childStates.push_back(child);
        }
    }
    return (childStates);
}

bool Writer_t::parseChildExits(State_t* currentState, size_t indentLevel, const State_Id_t topState, const bool didPreviousWrite)
{
    bool didWrite = didPreviousWrite;

    std::vector<State_t*> children = getChildStates(currentState);
    const size_t nChildren = children.size();

    if (0 == nChildren)
    {
        // need to detect here if any state from current to top state has exit
        // actions..
        State_t* tmpState = currentState;
        bool hasExitAction = false;
        while (topState != tmpState->id)
        {
            if (hasExitStatement(tmpState->id))
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
                out_c << getIndent(indentLevel) << "/* Handle super-step exit. */" << std::endl;
            }
            out_c << getIndent(indentLevel) << getIfElseIf(didWrite ? 1 : 0) << " (" << styler.getStateName(currentState) << " == state)" << std::endl;
            out_c << getIndent(indentLevel) << "{" << std::endl;
            indentLevel++;

            if (hasExitStatement(currentState->id))
            {
                out_c << getIndent(indentLevel) << styler.getStateExit(currentState) << "();" << std::endl;
            }

            if (config.doTracing)
            {
                out_c << getIndent(indentLevel) << getTraceCall_exit(currentState) << std::endl;
            }

            // go up to the top
            while (topState != currentState->id)
            {
                currentState = reader.getStateById(currentState->parent);
                if (hasExitStatement(currentState->id))
                {
                    out_c << getIndent(indentLevel) << styler.getStateExit(currentState) << "();" << std::endl;
                }
                if (config.doTracing)
                {
                    out_c << getIndent(indentLevel) << getTraceCall_exit(currentState) << std::endl;
                }
            }
            indentLevel--;
            out_c << getIndent(indentLevel) << "}" << std::endl;
            didWrite = true;
        }
    }
    else
    {
        for (size_t j = 0; j < nChildren; j++)
        {
            State_t* child = children[j];
            didWrite = parseChildExits(child, indentLevel, topState, didWrite);
        }
    }

    return (didWrite);
}

bool Writer_t::hasEntryStatement(const State_Id_t stateId)
{
    if (0u < reader.getDeclCount(stateId, Declaration_Entry))
    {
        return (true);
    }
    else
    {
        for (size_t j = 0; j < reader.getTransitionCountFromStateId(stateId); j++)
        {
            if (reader.getTransitionFrom(stateId, j)->event.isTimeEvent)
            {
                return (true);
            }
        }
    }
    return (false);
}

bool Writer_t::hasExitStatement(const State_Id_t stateId)
{
    if (0u < reader.getDeclCount(stateId, Declaration_Exit))
    {
        return (true);
    }
    else
    {
        for (size_t j = 0; j < reader.getTransitionCountFromStateId(stateId); j++)
        {
            if (reader.getTransitionFrom(stateId, j)->event.isTimeEvent)
            {
                return (true);
            }
        }
    }
    return (false);
}


std::string Writer_t::getTraceCall_entry(const State_t* state)
{
    return styler.getTraceEntry() + "(" + styler.getStateName(state) + ");";
}

std::string Writer_t::getTraceCall_exit(const State_t* state)
{
    return styler.getTraceExit() + "(" + styler.getStateName(state) + ");";
}

std::vector<State_t*> Writer_t::findEntryState(State_t* in)
{
    // this function will check if the in state contains an initial sub-state,
    // and follow the path until a state is reached that does not contain an
    // initial sub-state.
    std::vector<State_t*> states;
    states.push_back(in);

    // check if the state contains any initial sub-state
    bool foundNext = true;
    while (foundNext)
    {
        foundNext = false;
        for (size_t i = 0; i < reader.getStateCount(); i++)
        {
            State_t* tmp = reader.getState(i);
            if ((in->id != tmp->id) && (in->id == tmp->parent) && (0 == tmp->name.compare("initial")))
            {
                // a child state was found that is an initial state. Get transition
                // from this initial state, it should be one and only one.
                Transition_t* tr = reader.getTransitionFrom(tmp->id, 0);
                if (nullptr == tr)
                {
                    errorReport("Initial state in [" + styler.getStateName(in) + "] as no transitions.", __LINE__);
                }
                else
                {
                    // get the transition
                    tmp = reader.getStateById(tr->stB);
                    if (nullptr == tmp)
                    {
                        errorReport("Initial state in [" + styler.getStateName(in) + "] has no target.", __LINE__);
                    }
                    else
                    {
                        // recursively check the state, we can do this by
                        // exiting the for loop but continuing from the top.
                        states.push_back(tmp);
                        in = tmp;

                        // if the state is a choice, we need to stop here.
                        if (tmp->isChoice)
                        {
                            foundNext = false;
                        }
                        else
                        {
                            foundNext = true;
                        }
                    }
                }
            }
        }
    }

    return (states);
}

std::vector<State_t*> Writer_t::findFinalState(State_t* in)
{
    // this function will check if the in state has an outgoing transition to
    // a final state and follow the path until a state is reached that does not
    // contain an initial sub-state.
    std::vector<State_t*> states;
    states.push_back(in);

    // find parent to this state
    bool foundNext = true;
    while (foundNext)
    {
        foundNext = false;
        for (size_t i = 0; i < reader.getStateCount(); i++)
        {
            State_t* tmp = reader.getState(i);
            if ((in->id != tmp->id) && (in->parent == tmp->id) && (0 != tmp->name.compare("initial")))
            {
                // a parent state was found that is not an initial state. Check for
                // any outgoing transitions from this state to a final state. Store
                // the id of the tmp state, since we will reuse this pointer.
                const State_Id_t tmpId = tmp->id;
                for (size_t j = 0; j < reader.getTransitionCountFromStateId(tmpId); j++)
                {
                    Transition_t* tr = reader.getTransitionFrom(tmp->id, j);
                    tmp = reader.getStateById(tr->stB);
                    if ((nullptr != tmp) && (0 == tmp->name.compare("final")))
                    {
                        // recursively check the state
                        states.push_back(tmp);
                        in = tmp;

                        // if this state is a choice, we need to stop there.
                        if (tmp->isChoice)
                        {
                            foundNext = false;
                        }
                        else
                        {
                            foundNext = true;
                        }
                    }
                }
            }
        }
    }

    return (states);
}

std::vector<State_t*> Writer_t::findInitState()
{
    std::vector<State_t*> states;

    for (size_t i = 0; i < reader.getStateCount(); i++)
    {
        State_t* state = reader.getState(i);
        if (0 == state->name.compare("initial"))
        {
            // check if this is top initial state
            if (0 == state->parent)
            {
                // this is the top initial, find transition from this idle
                Transition_t* tr = reader.getTransitionFrom(state->id, 0);
                if (nullptr == tr)
                {
                    errorReport("No transition from initial state", __LINE__);
                }
                else
                {
                    // check target state (from top)
                    State_t* trStB = reader.getStateById(tr->stB);
                    if (nullptr == trStB)
                    {
                        errorReport("Transition to null state", __LINE__);
                    }
                    else
                    {
                        // get state where it stops.
                        states = findEntryState(trStB);
                    }
                }
            }
        }
    }

    return (states);
}

std::string Writer_t::getIndent(const size_t level)
{
    std::string s = "";
    for (size_t i = 0; i < level; i++)
    {
        s += "    ";
    }
    return (s);
}

std::string Writer_t::getIfElseIf(const size_t i)
{
    std::string s = "if";
    if (0 != i)
    {
        s = "else if";
    }
    return (s);
}
