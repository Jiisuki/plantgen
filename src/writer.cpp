#include <iostream>
#include <fstream>
#include <sstream>
#include <reader.hpp>
#include <writer.hpp>

Writer_t::Writer_t()
{
    this->reader = NULL;
    this->styler = NULL;
    this->config.doTracing = false;
    this->config.parentFirstExecution = false;
    this->config.useSimpleNames = false;
    this->config.verbose = false;
}

Writer_t::~Writer_t()
{
    if (NULL != this->reader)
    {
        delete this->reader;
    }
    if (NULL != this->styler)
    {
        delete this->styler;
    }
    if (this->out_c.is_open())
    {
        this->out_c.close();
    }
    if (this->out_h.is_open())
    {
        this->out_h.close();
    }
}

void Writer_t::enableParentFirstExecution(void)
{
    this->config.parentFirstExecution = true;
}

void Writer_t::enableSimpleNames(void)
{
    this->config.useSimpleNames = true;
}

void Writer_t::enableTracing(void)
{
    this->config.doTracing = true;
}

void Writer_t::enableVerbose(void)
{
    this->config.verbose = true;
}

void Writer_t::generateCode(const std::string filename, const std::string outdir)
{
    /* Create reader. */
    this->reader = new Reader_t(filename, this->config.verbose);
    if (NULL == this->reader)
    {
        this->errorReport("Allocation error", __LINE__);
        return;
    }

    /* Create styler. */
    this->styler = new Style_t(this->reader);
    if (NULL == this->styler)
    {
        this->errorReport("Allocation error", __LINE__);
        return;
    }

    if (this->config.useSimpleNames)
    {
        this->styler->enableSimpleNames();
    }
    else
    {
        this->styler->disableSimpleNames();
    }

    const std::string outfile_c = outdir + reader->getModelName() + ".c";
    const std::string outfile_h = outdir + reader->getModelName() + ".h";

    if (this->config.verbose)
    {
        std::cout << "Generating code from '" << filename << "' > '" << outfile_c << "' and '" << outfile_h << "' ..." << std::endl;
    }

    this->out_c.open(outfile_c);
    this->out_h.open(outfile_h);

    if (!this->out_c.is_open() || !this->out_h.is_open())
    {
        this->errorReport("Failed to open output files, does directory exist?", __LINE__);
        return;
    }

    this->out_h << "/** @file" << std::endl;
    this->out_h << " *  @brief Interface to the " << reader->getModelName() << " state machine." << std::endl;
    this->out_h << " *" << std::endl;
    this->out_h << " *  @startuml" << std::endl;
    for (size_t i = 0; i < reader->getUmlLineCount(); i++)
    {
        this->out_h << " *  " << reader->getUmlLine(i) << std::endl;
    }
    this->out_h << " *  @enduml" << std::endl;
    this->out_h << " */" << std::endl << std::endl;

    this->out_h << getIndent(0) << "#include <stdint.h>" << std::endl;
    this->out_h << getIndent(0) << "#include <stdbool.h>" << std::endl;
    this->out_h << getIndent(0) << "#include <stddef.h>" << std::endl;
    for (size_t i = 0; i < reader->getImportCount(); i++)
    {
        Import_t* imp = reader->getImport(i);
        this->out_h << getIndent(0) << "#include ";
        if (imp->isGlobal)
        {
            this->out_h << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            this->out_h << "\"" << imp->name << "\"" << std::endl;
        }
    }
    this->out_h << std::endl;

    // write all states to .h
    this->decl_stateList();

    // write all event types
    this->decl_eventList();

    // write all variables
    this->decl_variableList();

    // write main declaration
    this->decl_stateMachine();

    // write required implementation
    if (this->config.doTracing)
    {
        this->out_h << "/* The state machine uses tracing, therefore the following functions" << std::endl;
        this->out_h << " * are required to be implemented by the user. */" << std::endl;
        this->proto_traceEntry();
        this->proto_traceExit();
        this->out_h << std::endl;
    }

    // write init function
    this->out_h << "/** @brief Initialises the state machine. */" << std::endl;
    this->out_h << "void " << reader->getModelName() << "_init(" << this->styler->getHandleType() << " handle);" << std::endl << std::endl;

    // write time tick function
    this->proto_timeTick();

    // write all raise event function prototypes
    this->proto_raiseInEvent();
    this->proto_checkOutEvent();

    this->proto_getVariable();

    // write header to .c
    this->out_c << getIndent(0) << "#include <stdint.h>" << std::endl;
    this->out_c << getIndent(0) << "#include <stdbool.h>" << std::endl;
    this->out_c << getIndent(0) << "#include <stddef.h>" << std::endl;
    this->out_c << getIndent(0) << "#include \"" << reader->getModelName() << ".h\"" << std::endl;

    for (size_t i = 0; i < reader->getImportCount(); i++)
    {
        Import_t* imp = reader->getImport(i);
        this->out_c << getIndent(0) << "#include ";
        if (imp->isGlobal)
        {
            this->out_c << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            this->out_c << "\"" << imp->name << "\"" << std::endl;
        }
    }
    this->out_c << std::endl;

    // declare prototypes for global run cycle, clear in events, etc.
    this->out_c << "static void " << this->styler->getTopRunCycle() << "(" << this->styler->getHandleType() << " handle);" << std::endl;
    this->proto_clearEvents();
    this->proto_runCycle();
    this->proto_entryAction();
    this->proto_exitAction();
    this->proto_raiseOutEvent();
    this->proto_raiseInternalEvent();

    // find first state on init
    const std::vector<State_t*> firstState = this->findInitState();

    // write init implementation
    this->impl_init(firstState);

    // write all raise event functions
    this->impl_raiseInEvent();
    this->impl_checkOutEvent();
    this->impl_getVariable();
    this->impl_timeTick();
    this->impl_clearEvents();
    this->impl_topRunCycle();
    this->impl_runCycle();
    this->impl_entryAction();
    this->impl_exitAction();
    this->impl_raiseOutEvent();
    this->impl_raiseInternalEvent();

    /* end with a new line. */
    this->out_c << std::endl;
    this->out_h << std::endl;

    /* close streams. */
    this->out_c.close();
    this->out_h.close();
}

void Writer_t::errorReport(std::string str, unsigned int line)
{
    std::string fname = __FILE__;
    const size_t last_slash = fname.find_last_of("/");
    std::cout << "ERR: " << str << " - " << fname.substr(last_slash+1) << ": " << line << std::endl;
}

void Writer_t::proto_runCycle(void)
{
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
        {
            // no run cycle for initial or final states.
        }
        else if (state->isChoice)
        {
            // no run cycle for choices.
        }
        else
        {
            this->out_c << getIndent(0) << "static bool " << this->styler->getStateRunCycle(state) << "(" << this->styler->getHandleType() << " handle, const bool tryTransition);" << std::endl;
        }
    }
    this->out_c << std::endl;
}

void Writer_t::proto_entryAction(void)
{
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if ((0 != state->name.compare("initial")) && this->hasEntryStatement(state->id))
        {
            this->out_c << getIndent(0) << "static void " << this->styler->getStateEntry(state) << "(" << this->styler->getHandleType() << " handle);" << std::endl;
        }
    }
    this->out_c << std::endl;
}

void Writer_t::proto_exitAction(void)
{
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if ((0 != state->name.compare("initial")) && this->hasExitStatement(state->id))
        {
            this->out_c << getIndent(0) << "static void " << this->styler->getStateExit(state) << "(" << this->styler->getHandleType() << " handle);" << std::endl;
        }
    }
    this->out_c << std::endl;
}

void Writer_t::proto_raiseInEvent(void)
{
    for (size_t i = 0; i < reader->getInEventCount(); i++)
    {
        Event_t* ev = reader->getInEvent(i);
        this->out_h << getIndent(0) << "void " << this->styler->getEventRaise(ev) << "(" << this->styler->getHandleType() << " handle";
        if (ev->requireParameter)
        {
            this->out_h << ", const " << ev->parameterType << " value";
        }
        this->out_h << ");" << std::endl;
    }
    this->out_h << std::endl;
}

void Writer_t::proto_raiseOutEvent(void)
{
    for (size_t i = 0; i < reader->getOutEventCount(); i++)
    {
        Event_t* ev = reader->getOutEvent(i);
        this->out_c << getIndent(0) << "static void " << this->styler->getEventRaise(ev) << "(" << this->styler->getHandleType() << " handle";
        if (ev->requireParameter)
        {
            this->out_c << ", const " << ev->parameterType << " value";
        }
        this->out_c << ");" << std::endl;
    }
    this->out_c << std::endl;
}

void Writer_t::proto_raiseInternalEvent(void)
{
    for (size_t i = 0; i < reader->getInternalEventCount(); i++)
    {
        Event_t* ev = reader->getInternalEvent(i);
        this->out_c << getIndent(0) << "static void " << this->styler->getEventRaise(ev) << "(" << this->styler->getHandleType() << " handle";
        if (ev->requireParameter)
        {
            this->out_c << ", const " << ev->parameterType << " value";
        }
        this->out_c << ");" << std::endl;
    }
    this->out_c << std::endl;
}

void Writer_t::proto_timeTick(void)
{
    if (0 < reader->getTimeEventCount())
    {
        this->out_h << getIndent(0) << "void " << this->styler->getTimeTick() << "(" << this->styler->getHandleType() << " handle, const size_t timeElapsed_ms);" << std::endl << std::endl;
    }
}

void Writer_t::proto_checkOutEvent(void)
{
    for (size_t i = 0; i < reader->getOutEventCount(); i++)
    {
        Event_t* ev = reader->getOutEvent(i);
        this->out_h << "bool " << this->styler->getEventIsRaised(ev) << "(const " << this->styler->getHandleType() << " handle);" << std::endl;
    }
    if (0 < reader->getOutEventCount())
    {
        this->out_h << std::endl;
    }
}

void Writer_t::proto_clearEvents(void)
{
    if (0 < reader->getInEventCount())
    {
        this->out_c << "static void clearInEvents(" << this->styler->getHandleType() << " handle);" << std::endl;
    }

    if (0 < reader->getTimeEventCount())
    {
        this->out_c << "static void clearTimeEvents(" << this->styler->getHandleType() << " handle);" << std::endl;
    }

    if (0 < reader->getOutEventCount())
    {
        this->out_c << "static void clearOutEvents(" << this->styler->getHandleType() << " handle);" << std::endl;
    }
}

void Writer_t::proto_getVariable(void)
{
    for (size_t i = 0; i < reader->getVariableCount(); i++)
    {
        Variable_t* var = reader->getPublicVariable(i);
        if (NULL != var)
        {
            this->out_h << var->type << " " << this->styler->getVariable(var) << "(const " << this->styler->getHandleType() << " handle);" << std::endl;
        }
    }
    this->out_h << std::endl;
}

void Writer_t::proto_traceEntry(void)
{
    this->out_h << "extern void " << this->styler->getTraceEntry() << "(const " << this->styler->getStateType() << " state);" << std::endl;
}

void Writer_t::proto_traceExit(void)
{
    this->out_h << "extern void " << this->styler->getTraceExit() << "(const " << this->styler->getStateType() << " state);" << std::endl;
}

void Writer_t::decl_stateList(void)
{
    this->out_h << getIndent(0) << "typedef enum" << std::endl;
    this->out_h << getIndent(0) << "{" << std::endl;
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if (NULL != state)
        {
            /* Only write down state on actual states that the machine may stay in. */
            if ((0 != state->name.compare("initial")) && (0 != state->name.compare("final")) && (!state->isChoice))
            {
                this->out_h << getIndent(1) << this->styler->getStateName(state) << "," << std::endl;
            }
        }
    }
    this->out_h << getIndent(0) << "} " << reader->getModelName() << "_State_t;" << std::endl << std::endl;
}

void Writer_t::decl_eventList(void)
{
    const size_t nInEvents = reader->getInEventCount();
    const size_t nOutEvents = reader->getOutEventCount();
    const size_t nTimeEvents = reader->getTimeEventCount();
    const size_t nInternalEvents = reader->getInternalEventCount();

    if ((0 == nInEvents) && (0 == nTimeEvents) && (0 == nOutEvents) && (0 == nInternalEvents))
    {
        // don't write any
    }
    else
    {
        if (0 < nTimeEvents)
        {
            this->out_h << getIndent(0) << "typedef struct" << std::endl;
            this->out_h << getIndent(0) << "{" << std::endl;
            this->out_h << getIndent(1) << "bool isStarted;" << std::endl;
            this->out_h << getIndent(1) << "bool isRaised;" << std::endl;
            this->out_h << getIndent(1) << "bool isPeriodic;" << std::endl;
            this->out_h << getIndent(1) << "size_t timeout_ms;" << std::endl;
            this->out_h << getIndent(1) << "size_t expireTime_ms;" << std::endl;
            this->out_h << getIndent(0) << "} " << reader->getModelName() << "_TimeEvent_t;" << std::endl << std::endl;
        }

        this->out_h << getIndent(0) << "typedef struct" << std::endl;
        this->out_h << getIndent(0) << "{" << std::endl;
        if (0 < nInEvents)
        {
            this->out_h << getIndent(1) << "struct" << std::endl;
            this->out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nInEvents; i++)
            {
                Event_t* ev = reader->getInEvent(i);
                if (NULL != ev)
                {
                    this->out_h << getIndent(2) << "bool " << ev->name << "_isRaised;" << std::endl;
                    if (ev->requireParameter)
                    {
                        this->out_h << getIndent(2) << ev->parameterType << " " << ev->name << "_value;" << std::endl;
                    }
                }
            }
            this->out_h << getIndent(1) << "} inEvents;" << std::endl;
        }
        if (0 < nOutEvents)
        {
            this->out_h << getIndent(1) << "struct" << std::endl;
            this->out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nOutEvents; i++)
            {
                Event_t* ev = reader->getOutEvent(i);
                if (NULL != ev)
                {
                    this->out_h << getIndent(2) << "bool " << ev->name << "_isRaised;" << std::endl;
                    if (ev->requireParameter)
                    {
                        this->out_h << getIndent(2) << ev->parameterType << " " << ev->name << "_value;" << std::endl;
                    }
                }
            }
            this->out_h << getIndent(1) << "} outEvents;" << std::endl;
        }
        // check for time events
        if (0 < nTimeEvents)
        {
            this->out_h << getIndent(1) << "struct" << std::endl;
            this->out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nTimeEvents; i++)
            {
                Event_t* ev = reader->getTimeEvent(i);
                if (NULL != ev)
                {
                    this->out_h << getIndent(2) << reader->getModelName() << "_TimeEvent_t " << ev->name << ";" << std::endl;
                }
            }
            this->out_h << getIndent(1) << "} timeEvents;" << std::endl;
        }
        // check for internal events, these have no values.
        if (0 < nInternalEvents)
        {
            this->out_h << getIndent(1) << "struct" << std::endl;
            this->out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nInternalEvents; i++)
            {
                Event_t* ev = reader->getInternalEvent(i);
                if (NULL != ev)
                {
                    this->out_h << getIndent(2) << "bool " << ev->name << "_isRaised;" << std::endl;
                }
            }
            this->out_h << getIndent(1) << "} internalEvents;" << std::endl;
        }
        this->out_h << getIndent(0) << "} " << reader->getModelName() << "_EventList_t;" << std::endl;
        this->out_h << std::endl;
    }
}

void Writer_t::decl_variableList(void)
{
    const size_t nPrivate = reader->getPrivateVariableCount();
    const size_t nPublic  = reader->getPublicVariableCount();

    if ((0 == nPrivate) && (0 == nPublic))
    {
        // no variables!
    }
    else
    {
        this->out_h << getIndent(0) << "typedef struct" << std::endl;
        this->out_h << getIndent(0) << "{" << std::endl;

        // write private
        if (0 < nPrivate)
        {
            this->out_h << getIndent(1) << "struct" << std::endl;
            this->out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nPrivate; i++)
            {
                Variable_t* var = reader->getPrivateVariable(i);
                if (var->isPrivate)
                {
                    this->out_h << getIndent(2) << var->type << " " << var->name << ";" << std::endl;
                }
            }
            this->out_h << getIndent(1) << "} internal;" << std::endl;
        }

        // write public
        if (0 < nPublic)
        {
            this->out_h << getIndent(1) << "struct" << std::endl;
            this->out_h << getIndent(1) << "{" << std::endl;
            for (size_t i = 0; i < nPublic; i++)
            {
                Variable_t* var = reader->getPublicVariable(i);
                if (true != var->isPrivate)
                {
                    this->out_h << getIndent(2) << var->type << " " << var->name << ";" << std::endl;
                }
            }
            this->out_h << getIndent(1) << "} public;" << std::endl;
        }

        this->out_h << getIndent(0) << "} " << reader->getModelName() << "_Variables_t;" << std::endl << std::endl;
    }
}

void Writer_t::decl_stateMachine(void)
{
    // write internal structure
    this->out_h << getIndent(0) << "typedef struct" << std::endl;
    this->out_h << getIndent(0) << "{" << std::endl;
    this->out_h << getIndent(1) << reader->getModelName() << "_State_t state;" << std::endl;
    this->out_h << getIndent(1) << reader->getModelName() << "_EventList_t events;" << std::endl;
    this->out_h << getIndent(1) << reader->getModelName() << "_Variables_t variables;" << std::endl;
    if (0 < reader->getTimeEventCount())
    {
        // time now counter
        this->out_h << getIndent(1) << "size_t timeNow_ms;" << std::endl;
    }
    this->out_h << getIndent(0) << "} " << reader->getModelName() << "_t;" << std::endl << std::endl;

    this->out_h << "typedef " << reader->getModelName() << "_t* " << this->styler->getHandleType() << ";" << std::endl << std::endl;
}

void Writer_t::impl_init(const std::vector<State_t*> firstState)
{
    this->out_c << "void " << reader->getModelName() << "_init(" << this->styler->getHandleType() << " handle)" << std::endl;
    this->out_c << "{" << std::endl;

    // write variable inits
    this->out_c << getIndent(1) << "/* Initialise variables. */" << std::endl;
    for (size_t i = 0; i < reader->getVariableCount(); i++)
    {
        Variable_t* var = reader->getVariable(i);
        if (var->isPrivate)
        {
            this->out_c << getIndent(1) << "handle->variables.internal.";
        }
        else
        {
            this->out_c << getIndent(1) << "handle->variables.exported.";
        }
        this->out_c << var->name << " = ";
        if (var->specificInitialValue)
        {
            this->out_c << var->initialValue << ";" << std::endl;
        }
        else
        {
            this->out_c << "0;" << std::endl;
        }
    }
    this->out_c << std::endl;

    // write event clearing
    this->out_c << getIndent(1) << "/* Clear events. */" << std::endl;
    this->out_c << getIndent(1) << "clearInEvents(handle);" << std::endl;
    if (0 < firstState.size())
    {
        this->out_c << std::endl;
        this->out_c << getIndent(1) << "/* Set initial state. */" << std::endl;
        State_t* targetState = NULL;
        for (size_t i = 0; i < firstState.size(); i++)
        {
            targetState = firstState[i];
            if (this->hasEntryStatement(targetState->id))
            {
                // write entry call
                this->out_c << getIndent(1) << this->styler->getStateEntry(targetState) << "(handle);" << std::endl;
            }
        }
        this->out_c << getIndent(1) << "handle->state = " << this->styler->getStateName(targetState) << ";" << std::endl;
#if 0
        if (writerConfig.doTracing)
        {
            writer->out_c << getIndent(1) << getTraceCall_entry(reader) << std::endl;
        }
#endif
    }
    this->out_c << "}" << std::endl << std::endl;
}

void Writer_t::impl_raiseInEvent(void)
{
    for (size_t i = 0; i < reader->getInEventCount(); i++)
    {
        Event_t* ev = reader->getInEvent(i);
        this->out_c << getIndent(0) << "void " << this->styler->getEventRaise(ev) << "(" << this->styler->getHandleType() << " handle";
        if (ev->requireParameter)
        {
            this->out_c << ", const " << ev->parameterType << " value";
        }
        this->out_c << ")" << std::endl;
        this->out_c << getIndent(0) << "{" << std::endl;
        if (ev->requireParameter)
        {
            this->out_c << getIndent(1) << "handle->events.inEvents." << ev->name << "_value = value;" << std::endl;
        }
        this->out_c << getIndent(1) << "handle->events.inEvents." << ev->name << "_isRaised = true;" << std::endl;
        this->out_c << getIndent(1) << this->styler->getTopRunCycle() << "(handle);" << std::endl;
        this->out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_raiseOutEvent(void)
{
    for (size_t i = 0; i < reader->getOutEventCount(); i++)
    {
        Event_t* ev = reader->getOutEvent(i);
        this->out_c << getIndent(0) << "static void " << this->styler->getEventRaise(ev) << "(" << this->styler->getHandleType() << " handle";
        if (ev->requireParameter)
        {
            this->out_c << ", const " << ev->parameterType << " value";
        }
        this->out_c << ")" << std::endl;
        this->out_c << getIndent(0) << "{" << std::endl;
        if (ev->requireParameter)
        {
            this->out_c << getIndent(1) << "handle->events.outEvents." << ev->name << "_value = value;" << std::endl;
        }
        this->out_c << getIndent(1) << "handle->events.outEvents." << ev->name << "_isRaised = true;" << std::endl;
        this->out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_raiseInternalEvent(void)
{
    for (size_t i = 0; i < reader->getInternalEventCount(); i++)
    {
        Event_t* ev = reader->getInternalEvent(i);
        this->out_c << getIndent(0) << "static void " << this->styler->getEventRaise(ev) << "(" << this->styler->getHandleType() << " handle";
        if (ev->requireParameter)
        {
            this->out_c << ", const " << ev->parameterType << " value";
        }
        this->out_c << ")" << std::endl;
        this->out_c << getIndent(0) << "{" << std::endl;
        if (ev->requireParameter)
        {
            this->out_c << getIndent(1) << "handle->events.internalEvents." << ev->name << "_value = value;" << std::endl;
        }
        this->out_c << getIndent(1) << "handle->events.internalEvents." << ev->name << "_isRaised = true;" << std::endl;
        this->out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_checkOutEvent(void)
{
    for (size_t i = 0; i < reader->getOutEventCount(); i++)
    {
        Event_t* ev = reader->getOutEvent(i);
        this->out_c << "bool " << this->styler->getEventIsRaised(ev) << "(const " << this->styler->getHandleType() << " handle)" << std::endl;
        this->out_c << "{" << std::endl;
        this->out_c << getIndent(1) << "return (handle->events.outEvents." << ev->name << "_isRaised);" << std::endl;
        this->out_c << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_getVariable(void)
{
    for (size_t i = 0; i < reader->getVariableCount(); i++)
    {
        Variable_t* var = reader->getPublicVariable(i);
        if (NULL != var)
        {
            this->out_c << var->type << " " << this->styler->getVariable(var) << "(const " << this->styler->getHandleType() << " handle)" << std::endl;
            this->out_c << "{" << std::endl;
            this->out_c << getIndent(1) << "return (handle->variables.exported." << var->name << ");" << std::endl;
            this->out_c << "}" << std::endl << std::endl;
        }
    }
}

void Writer_t::impl_timeTick(void)
{
    if (0 < reader->getTimeEventCount())
    {
        this->out_c << getIndent(0) << "void " << this->styler->getTimeTick() << "(" << this->styler->getHandleType() << " handle, const size_t timeElapsed_ms)" << std::endl;
        this->out_c << getIndent(0) << "{" << std::endl;
        this->out_c << getIndent(1) << "handle->timeNow_ms += timeElapsed_ms;" << std::endl << std::endl;
        for (size_t i = 0; i < reader->getTimeEventCount(); i++)
        {
            Event_t* ev = reader->getTimeEvent(i);
            if (NULL != ev)
            {
                this->out_c << getIndent(1) << "if (handle->events.timeEvents." << ev->name << ".isStarted)" << std::endl;
                this->out_c << getIndent(1) << "{" << std::endl;
                this->out_c << getIndent(2) << "if (handle->events.timeEvents." << ev->name << ".expireTime_ms <= handle->timeNow_ms)" << std::endl;
                this->out_c << getIndent(2) << "{" << std::endl;
                this->out_c << getIndent(3) << "handle->events.timeEvents." << ev->name << ".isRaised = true;" << std::endl;
                this->out_c << getIndent(3) << "if (handle->events.timeEvents." << ev->name << ".isPeriodic)" << std::endl;
                this->out_c << getIndent(3) << "{" << std::endl;
                this->out_c << getIndent(4) << "handle->events.timeEvents." << ev->name << ".expireTime_ms += handle->events.timeEvents." << ev->name << ".timeout_ms;" << std::endl;
                this->out_c << getIndent(4) << "handle->events.timeEvents." << ev->name << ".isStarted = true;" << std::endl;
                this->out_c << getIndent(3) << "}" << std::endl;
                this->out_c << getIndent(3) << "else" << std::endl;
                this->out_c << getIndent(3) << "{" << std::endl;
                this->out_c << getIndent(4) << "handle->events.timeEvents." << ev->name << ".isStarted = false;" << std::endl;
                this->out_c << getIndent(3) << "}" << std::endl;
                this->out_c << getIndent(2) << "}" << std::endl;
                this->out_c << getIndent(1) << "}" << std::endl;
            }
        }
        this->out_c << std::endl;
        this->out_c << getIndent(1) << this->styler->getTopRunCycle() << "(handle);" << std::endl;
        this->out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_clearEvents(void)
{
    if (0 < reader->getInEventCount())
    {
        this->out_c << "static void clearInEvents(" << this->styler->getHandleType() << " handle)" << std::endl;
        this->out_c << "{" << std::endl;
        for (size_t i = 0; i < reader->getInEventCount(); i++)
        {
            Event_t* ev = reader->getInEvent(i);
            this->out_c << getIndent(1) << "handle->events.inEvents." << ev->name << "_isRaised = false;" << std::endl;
        }
        this->out_c << "}" << std::endl << std::endl;
    }

    if (0 < reader->getTimeEventCount())
    {
        this->out_c << "static void clearTimeEvents(" << this->styler->getHandleType() << " handle)" << std::endl;
        this->out_c << "{" << std::endl;
        for (size_t i = 0; i < reader->getTimeEventCount(); i++)
        {
            Event_t* ev = reader->getTimeEvent(i);
            this->out_c << getIndent(1) << "handle->events.timeEvents." << ev->name << ".isRaised = false;" << std::endl;
        }
        this->out_c << "}" << std::endl << std::endl;
    }

    if (0 < reader->getOutEventCount())
    {
        this->out_c << "static void clearOutEvents(" << this->styler->getHandleType() << " handle)" << std::endl;
        this->out_c << "{" << std::endl;
        for (size_t i = 0; i < reader->getOutEventCount(); i++)
        {
            Event_t* ev = reader->getOutEvent(i);
            this->out_c << getIndent(1) << "handle->events.outEvents." << ev->name << "_isRaised = false;" << std::endl;
        }
        this->out_c << "}" << std::endl << std::endl;
    }
}

void Writer_t::impl_topRunCycle(void)
{
    size_t writeNumber = 0;
    size_t indent = 0;
    this->out_c << "static void " << this->styler->getTopRunCycle() << "(" << this->styler->getHandleType() << " handle)" << std::endl;
    this->out_c << "{" << std::endl;
    indent++;
    if (0 < reader->getOutEventCount())
    {
        this->out_c << getIndent(indent) << "clearOutEvents(handle);" << std::endl << std::endl;
    }

    if (0 < reader->getInternalEventCount())
    {
        this->out_c << getIndent(indent) << "/* While an internal event is raised, perform loop. */" << std::endl;
        this->out_c << getIndent(indent) << "do" << std::endl;
        this->out_c << getIndent(indent) << "{" << std::endl;
        indent++;
    }

    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
        {
            // no handling on initial or final states.
        }
        else if (state->isChoice)
        {
            // no handling on choice.
        }
        else
        {
            this->out_c << getIndent(indent) << this->getIfElseIf(writeNumber) << " (handle->state == " << this->styler->getStateName(state) << ")" << std::endl;
            this->out_c << getIndent(indent) << "{" << std::endl;
            indent++;
            this->out_c << getIndent(indent) << this->styler->getStateRunCycle(state) << "(handle, true);" << std::endl;
            indent--;
            this->out_c << getIndent(indent) << "}" << std::endl;
            writeNumber++;
        }
    }
    if (0 < reader->getStateCount())
    {
        this->out_c << getIndent(indent) << "else" << std::endl;
        this->out_c << getIndent(indent) << "{" << std::endl;
        indent++;
        this->out_c << getIndent(indent) << "// possibly final state!" << std::endl;
        indent--;
        this->out_c << getIndent(indent) << "}" << std::endl;
    }

    // TODO: Check for pending internal events, then do another run..
    if (0 < reader->getInternalEventCount())
    {
        indent--;
        this->out_c << getIndent(indent) << "} while (false);" << std::endl << std::endl;
    }
    else
    {
        this->out_c << std::endl;
    }

    if (0 < reader->getInEventCount())
    {
        this->out_c << getIndent(indent) << "clearInEvents(handle);" << std::endl;
    }
    if (0 < reader->getTimeEventCount())
    {
        this->out_c << getIndent(indent) << "clearTimeEvents(handle);" << std::endl;
    }
    this->out_c << "}" << std::endl << std::endl;
}

void Writer_t::impl_runCycle(void)
{
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
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

            this->out_c << "static bool " << this->styler->getStateRunCycle(state) << "(" << this->styler->getHandleType() << " handle, const bool tryTransition)" << std::endl;
            this->out_c << "{" << std::endl;
            indentLevel++;

            // write comment declaration here if one exists.
            const size_t numCommentLines = reader->getDeclCount(state->id, Declaration_Comment);
            if (0 < numCommentLines)
            {
                isEmptyBody = false;
                for (size_t j = 0; j < numCommentLines; j++)
                {
                    State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_Comment, j);
                    this->out_c << getIndent(indentLevel) << "/* " << decl->declaration << " */" << std::endl;
                }
                this->out_c << std::endl;
            }

            this->out_c << getIndent(indentLevel) << "bool didTransition = tryTransition;" << std::endl;
            this->out_c << getIndent(indentLevel) << "if (tryTransition)" << std::endl;
            this->out_c << getIndent(indentLevel) << "{" << std::endl;
            indentLevel++;

            const size_t nOutTr = reader->getTransitionCountFromStateId(state->id);

            // write parent react
            State_t* parentState = reader->getStateById(state->parent);
            if (NULL != parentState)
            {
                isEmptyBody = false;
                {
                    this->out_c << getIndent(indentLevel) << "if (!" << this->styler->getStateRunCycle(parentState) << "(handle, tryTransition))" << std::endl;
                    this->out_c << getIndent(indentLevel) << "{" << std::endl;
                    indentLevel++;
                }
            }

            if (0 == nOutTr)
            {
                this->out_c << getIndent(indentLevel) << "didTransition = false;" << std::endl;
            }
            else
            {
                for (size_t j = 0; j < nOutTr; j++)
                {
                    Transition_t* tr = reader->getTransitionFrom(state->id, j);
                    if (0 == tr->event.name.compare("null"))
                    {
                        State_t* trStB = reader->getStateById(tr->stB);
                        if (NULL == trStB)
                        {
                            this->errorReport("Null transition!", __LINE__);
                        }
                        else if (0 != trStB->name.compare("final"))
                        {
                            this->errorReport("Null transition!", __LINE__);

                            // handle as a oncycle transition?
                            isEmptyBody = false;

                            this->out_c << getIndent(indentLevel) << getIfElseIf(j) << " (true)" << std::endl;
                            this->out_c << getIndent(indentLevel) << "{" << std::endl;
                            indentLevel++;

                            // if tracing is enabled
#if 0
                            if (writerConfig.doTracing)
                            {
                                writer->out_c << getIndent(indentLevel) << getTraceCall_exit(reader) << std::endl;
                            }
#endif

                            // is exit function exists
                            if (this->hasExitStatement(state->id))
                            {
                                this->out_c << getIndent(indentLevel) << this->styler->getStateExit(state) << "(handle);" << std::endl;
                            }

                            indentLevel--;
                            this->out_c << getIndent(indentLevel) << "}" << std::endl;
                        }
                    }
                    else
                    {
                        State_t* trStB = reader->getStateById(tr->stB);
                        if (NULL == trStB)
                        {
                            this->errorReport("Null transition!", __LINE__);
                        }
                        else
                        {
                            isEmptyBody = false;

                            if (tr->event.isTimeEvent)
                            {
                                if (tr->hasGuard)
                                {
                                    std::string guardStr = this->parseGuard(tr->guard);
                                    this->out_c << getIndent(indentLevel)
                                                  << getIfElseIf(j)
                                                  << " ((handle->events.timeEvents."
                                                  << tr->event.name
                                                  << ".isRaised) && ("
                                                  << guardStr
                                                  << "))"
                                                  << std::endl;
                                }
                                else
                                {
                                    this->out_c << getIndent(indentLevel)
                                                  << getIfElseIf(j)
                                                  << " (handle->events.timeEvents."
                                                  << tr->event.name
                                                  << ".isRaised)"
                                                  << std::endl;
                                }
                            }
                            else
                            {
                                if (tr->hasGuard)
                                {
                                    std::string guardStr = this->parseGuard(tr->guard);
                                    this->out_c << getIndent(indentLevel)
                                                  << getIfElseIf(j);
                                    if (Event_Direction_Incoming == tr->event.direction)
                                    {
                                        this->out_c << " ((handle->events.inEvents."
                                                      << tr->event.name
                                                      << "_isRaised) && (";
                                    }
                                    else if (Event_Direction_Internal == tr->event.direction)
                                    {
                                        this->out_c << " ((handle->events.internalEvents."
                                                      << tr->event.name
                                                      << "_isRaised) && (";
                                    }
                                    else
                                    {
                                        this->out_c << " ((handle->events.outEvents."
                                                      << tr->event.name
                                                      << "_isRaised) && (";
                                    }
                                    this->out_c << guardStr
                                                  << "))"
                                                  << std::endl;
                                }
                                else
                                {
                                    if (Event_Direction_Incoming == tr->event.direction)
                                    {
                                        this->out_c << getIndent(indentLevel) << getIfElseIf(j) << " (handle->events.inEvents." << tr->event.name << "_isRaised)" << std::endl;
                                    }
                                    else if (Event_Direction_Internal == tr->event.direction)
                                    {
                                        this->out_c << getIndent(indentLevel) << getIfElseIf(j) << " (handle->events.internalEvents." << tr->event.name << "_isRaised)" << std::endl;
                                    }
                                    else
                                    {
                                        this->out_c << getIndent(indentLevel) << getIfElseIf(j) << " (handle->events.outEvents." << tr->event.name << "_isRaised)" << std::endl;
                                    }
                                }
                            }
                            this->out_c << getIndent(indentLevel) << "{" << std::endl;
                            indentLevel++;

                            const bool didChildExits = this->parseChildExits(state, indentLevel, state->id, false);

                            if (didChildExits)
                            {
                                this->out_c << std::endl;
                            }
                            else
                            {
                                if (this->hasExitStatement(state->id))
                                {
                                    this->out_c << getIndent(indentLevel) << "/* Handle super-step exit. */" << std::endl;
                                    this->out_c << getIndent(indentLevel) << this->styler->getStateExit(state) << "(handle);" << std::endl;
                                }
                                if (this->config.doTracing)
                                {
                                    this->out_c << getIndent(indentLevel) << this->getTraceCall_exit(state) << std::endl;
                                }
                                /* Extra new-line */
                                if ((this->hasExitStatement(state->id)) || (this->config.doTracing))
                                {
                                    this->out_c << std::endl;
                                }
                            }

                            // TODO: do entry actins on all states entered
                            // towards the goal! Might needs some work..
                            std::vector<State_t*> enteredStates = this->findEntryState(trStB);

                            if (0 < enteredStates.size())
                            {
                                this->out_c << getIndent(indentLevel) << "/* Handle super-step entry. */" << std::endl;
                            }

                            State_t* finalState = NULL;
                            for (size_t j = 0; j < enteredStates.size(); j++)
                            {
                                finalState = enteredStates[j];

                                if (this->hasEntryStatement(finalState->id))
                                {
                                    this->out_c << getIndent(indentLevel) << this->styler->getStateEntry(finalState) << "(handle);" << std::endl;
                                }

                                if (this->config.doTracing)
                                {
                                    this->out_c << getIndent(indentLevel) << this->getTraceCall_entry(finalState) << std::endl;
                                }
                            }

                            // handle choice node?
                            if ((NULL != finalState) && (finalState->isChoice))
                            {
                                this->parseChoicePath(finalState, indentLevel);
                            }
                            else
                            {
                                this->out_c << getIndent(indentLevel) << "handle->state = " << this->styler->getStateName(finalState) << ";" << std::endl;
                            }

                            indentLevel--;
                            this->out_c << getIndent(indentLevel) << "}" << std::endl;
                        }
                    }
                }

                this->out_c << getIndent(indentLevel) << "else" << std::endl;
                this->out_c << getIndent(indentLevel) << "{" << std::endl;
                indentLevel++;
                this->out_c << getIndent(indentLevel) << "didTransition = false;" << std::endl;
                indentLevel--;
                this->out_c << getIndent(indentLevel) << "}" << std::endl;
            }

            while (1 < indentLevel)
            {
                indentLevel--;
                this->out_c << getIndent(indentLevel) << "}" << std::endl;
            }

            if (isEmptyBody)
            {
                this->out_c << getIndent(indentLevel) << "(void) handle;" << std::endl;
            }

            this->out_c << getIndent(indentLevel) << "return (didTransition);" << std::endl;
            this->out_c << "}" << std::endl << std::endl;
        }
    }
}

void Writer_t::impl_entryAction(void)
{
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader->getDeclCount(state->id, Declaration_Entry);
            size_t numTimeEv = 0;
            for (size_t j = 0; j < reader->getTransitionCountFromStateId(state->id); j++)
            {
                Transition_t* tr = reader->getTransitionFrom(state->id, j);
                if ((NULL != tr) && (tr->event.isTimeEvent))
                {
                    numTimeEv++;
                }
            }

            if ((0 < numDecl) || (0 < numTimeEv))
            {
                this->out_c << getIndent(0) << "static void " << this->styler->getStateEntry(state) << "(" << this->styler->getHandleType() << " handle)" << std::endl;
                this->out_c << getIndent(0) << "{" << std::endl;

                // start timers
                size_t writeIndex = 0;
                for (size_t j = 0; j < reader->getTransitionCountFromStateId(state->id); j++)
                {
                    Transition_t* tr = reader->getTransitionFrom(state->id, j);
                    if ((NULL != tr) && (tr->event.isTimeEvent))
                    {
                        this->out_c << getIndent(1) << "/* Start timer " << tr->event.name << " with timeout of " << tr->event.expireTime_ms << " ms. */" << std::endl;
                        this->out_c << getIndent(1) << "handle->events.timeEvents." << tr->event.name << ".timeout_ms = " << tr->event.expireTime_ms << ";" << std::endl;
                        this->out_c << getIndent(1) << "handle->events.timeEvents." << tr->event.name << ".expireTime_ms = handle->timeNow_ms + " << tr->event.expireTime_ms << ";" << std::endl;
                        this->out_c << getIndent(1) << "handle->events.timeEvents." << tr->event.name << ".isPeriodic = " << (tr->event.isPeriodic ? "true;" : "false;") << std::endl;
                        this->out_c << getIndent(1) << "handle->events.timeEvents." << tr->event.name << ".isStarted = true;" << std::endl;
                        writeIndex++;
                        if (writeIndex < numTimeEv)
                        {
                            this->out_c << std::endl;
                        }
                    }
                }

                if ((0 < numDecl) && (0 < numTimeEv))
                {
                    // add a space between the parts
                    this->out_c << std::endl;
                }

                for (size_t j = 0; j < numDecl; j++)
                {
                    State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_Entry, j);
                    if (Declaration_Entry == decl->type)
                    {
                        this->parseDeclaration(decl->declaration);
                    }
                }
                this->out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
}

void Writer_t::impl_exitAction(void)
{
    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader->getDeclCount(state->id, Declaration_Exit);
            size_t numTimeEv = 0;
            for (size_t j = 0; j < reader->getTransitionCountFromStateId(state->id); j++)
            {
                Transition_t* tr = reader->getTransitionFrom(state->id, j);
                if ((NULL != tr) && (tr->event.isTimeEvent))
                {
                    numTimeEv++;
                }
            }

            if ((0 < numDecl) || (0 < numTimeEv))
            {
                this->out_c << getIndent(0) << "static void " << this->styler->getStateExit(state) << "(" << this->styler->getHandleType() << " handle)" << std::endl;
                this->out_c << getIndent(0) << "{" << std::endl;

                // stop timers
                for (size_t j = 0; j < reader->getTransitionCountFromStateId(state->id); j++)
                {
                    Transition_t* tr = reader->getTransitionFrom(state->id, j);
                    if ((NULL != tr) && (tr->event.isTimeEvent))
                    {
                        this->out_c << getIndent(1) << "handle->events.timeEvents." << tr->event.name << ".isStarted = false;" << std::endl;
                    }
                }

                if ((0 < numDecl) && (0 < numTimeEv))
                {
                    // add a space between the parts
                    this->out_c << std::endl;
                }

                if (0 < numDecl)
                {
                    for (size_t j = 0; j < numDecl; j++)
                    {
                        State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_Exit, j);
                        if (Declaration_Exit == decl->type)
                        {
                            this->parseDeclaration(decl->declaration);
                        }
                    }
                }
                this->out_c << getIndent(0) << "}" << std::endl << std::endl;
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

void Writer_t::parseDeclaration(const std::string declaration)
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

                for (size_t i = 0; i < reader->getVariableCount(); i++)
                {
                    Variable_t* var = reader->getVariable(i);
                    if (0 == var->name.compare(replaceString))
                    {
                        wstr += "handle->variables.";
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
                    for (size_t i = 0; i < reader->getInEventCount(); i++)
                    {
                        Event_t* ev = reader->getInEvent(i);
                        if (0 == ev->name.compare(replaceString))
                        {
                            wstr += "handle->events.inEvents." + ev->name + "_value";
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
                        outstr += this->styler->getEventRaise(evName) + "(handle);";
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

    this->out_c << getIndent(1) << outstr << std::endl;
}

std::string Writer_t::parseGuard(const std::string guardStrRaw)
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

                for (size_t i = 0; i < reader->getVariableCount(); i++)
                {
                    Variable_t* var = reader->getVariable(i);
                    if (0 == var->name.compare(replaceString))
                    {
                        wstr += "handle->variables.";
                        if (var->isPrivate)
                        {
                            wstr += "private.";
                        }
                        else
                        {
                            wstr += "public.";
                        }
                        wstr += var->name;
                        isReplaced = true;
                        break;
                    }
                }

                if (!isReplaced)
                {
                    for (size_t i = 0; i < reader->getInEventCount(); i++)
                    {
                        Event_t* ev = reader->getInEvent(i);
                        if (0 == ev->name.compare(replaceString))
                        {
                            wstr += "handle->events.inEvents." + ev->name + ".param";
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
    this->out_c << std::endl << getIndent(indentLevel) << "/* Choice: " << state->name << " */" << std::endl;

    const size_t numChoiceTr = reader->getTransitionCountFromStateId(state->id);
    if (numChoiceTr < 2)
    {
        this->errorReport("Ony one transition from choice " + state->name, __LINE__);
    }
    else
    {
        Transition_t* defaultTr = NULL;
        size_t k = 0;
        for (size_t j = 0; j < numChoiceTr; j++)
        {
            Transition_t* tr = reader->getTransitionFrom(state->id, j);
            if (true != tr->hasGuard)
            {
                defaultTr = tr;
            }
            else
            {
                // handle if statement
                this->out_c << getIndent(indentLevel + 0) << this->getIfElseIf(k++) << " (" << this->parseGuard(tr->guard) << ")" << std::endl;
                this->out_c << getIndent(indentLevel + 0) << "{" << std::endl;
                State_t* guardedState = reader->getStateById(tr->stB);
                this->out_c << getIndent(indentLevel + 1) << "// goto: " << guardedState->name << std::endl;
                if (NULL == guardedState)
                {
                    this->errorReport("Invalid transition from choice " + state->name, __LINE__);
                }
                else
                {
                    std::vector<State_t*> enteredStates = this->findEntryState(guardedState);
                    State_t* finalState = NULL;
                    for (size_t j = 0; j < enteredStates.size(); j++)
                    {
                        finalState = enteredStates[j];

                        // TODO: fix call so send state name.
#if 0
                        if (writerConfig.doTracing)
                        {
                            this->out_c << getIndent(indentLevel + 1) << getTraceCall_entry(reader) << std::endl;
                        }
#endif

                        if (0 < reader->getDeclCount(finalState->id, Declaration_Entry))
                        {
                            this->out_c << getIndent(indentLevel + 1) << this->styler->getStateEntry(finalState) << "(handle);" << std::endl;
                        }
                    }
                    if (NULL != finalState)
                    {
                        if (finalState->isChoice)
                        {
                            // nest ..
                            this->parseChoicePath(finalState, indentLevel + 1);
                        }
                        else
                        {
                            this->out_c << getIndent(indentLevel + 1) << "handle->state = " << this->styler->getStateName(finalState) << ";" << std::endl;
                        }
                    }
                }
                this->out_c << getIndent(indentLevel + 0) << "}" << std::endl;
            }
        }
        if (NULL != defaultTr)
        {
            // write default transition.
            this->out_c << getIndent(indentLevel + 0) << "else" << std::endl;
            this->out_c << getIndent(indentLevel + 0) << "{" << std::endl;
            State_t* guardedState = reader->getStateById(defaultTr->stB);
            this->out_c << getIndent(indentLevel + 1) << "// goto: " << guardedState->name << std::endl;
            if (NULL == guardedState)
            {
                this->errorReport("Invalid transition from choice " + state->name, __LINE__);
            }
            else
            {
                std::vector<State_t*> enteredStates = this->findEntryState(guardedState);
                State_t* finalState = NULL;
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

                    if (0 < reader->getDeclCount(finalState->id, Declaration_Entry))
                    {
                        this->out_c << getIndent(indentLevel + 1) << this->styler->getStateEntry(finalState) << "(handle);" << std::endl;
                    }
                }
                if (NULL != finalState)
                {
                    if (finalState->isChoice)
                    {
                        // nest ..
                        this->parseChoicePath(finalState, indentLevel + 1);
                    }
                    else
                    {
                        this->out_c << getIndent(indentLevel + 1) << "handle->state = " << this->styler->getStateName(finalState) << ";" << std::endl;
                    }
                }
            }
            this->out_c << getIndent(indentLevel + 0) << "}" << std::endl;
        }
        else
        {
            this->errorReport("No default transition from " + state->name, __LINE__);
        }
    }
}

std::vector<State_t*> Writer_t::getChildStates(State_t* currentState)
{
    std::vector<State_t*> childStates;
    for (size_t j = 0; j < reader->getStateCount(); j++)
    {
        State_t* child = reader->getState(j);

        if ((NULL != child) &&
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

    std::vector<State_t*> children = this->getChildStates(currentState);
    const size_t nChildren = children.size();

    if (0 == nChildren)
    {
        // need to detect here if any state from current to top state has exit
        // actions..
        State_t* tmpState = currentState;
        bool hasExitAction = false;
        while (topState != tmpState->id)
        {
            if (this->hasExitStatement(tmpState->id))
            {
                hasExitAction = true;
                break;
            }
            tmpState = reader->getStateById(tmpState->parent);
        }

        if (hasExitAction)
        {
            if (!didWrite)
            {
                this->out_c << getIndent(indentLevel) << "/* Handle super-step exit. */" << std::endl;
            }
            this->out_c << getIndent(indentLevel) << this->getIfElseIf(didWrite ? 1 : 0) << " (" << this->styler->getStateName(currentState) << " == handle->state)" << std::endl;
            this->out_c << getIndent(indentLevel) << "{" << std::endl;
            indentLevel++;

            if (this->hasExitStatement(currentState->id))
            {
                this->out_c << getIndent(indentLevel) << this->styler->getStateExit(currentState) << "(handle);" << std::endl;
            }

            if (this->config.doTracing)
            {
                this->out_c << getIndent(indentLevel) << this->getTraceCall_exit(currentState) << std::endl;
            }

            // go up to the top
            while (topState != currentState->id)
            {
                currentState = reader->getStateById(currentState->parent);
                if (this->hasExitStatement(currentState->id))
                {
                    this->out_c << getIndent(indentLevel) << this->styler->getStateExit(currentState) << "(handle);" << std::endl;
                }
                if (this->config.doTracing)
                {
                    this->out_c << getIndent(indentLevel) << this->getTraceCall_exit(currentState) << std::endl;
                }
            }
            indentLevel--;
            this->out_c << getIndent(indentLevel) << "}" << std::endl;
            didWrite = true;
        }
    }
    else
    {
        for (size_t j = 0; j < nChildren; j++)
        {
            State_t* child = children[j];
            didWrite = this->parseChildExits(child, indentLevel, topState, didWrite);
        }
    }

    return (didWrite);
}

bool Writer_t::hasEntryStatement(const State_Id_t stateId)
{
    if (0u < reader->getDeclCount(stateId, Declaration_Entry))
    {
        return (true);
    }
    else
    {
        for (size_t j = 0; j < reader->getTransitionCountFromStateId(stateId); j++)
        {
            if (reader->getTransitionFrom(stateId, j)->event.isTimeEvent)
            {
                return (true);
            }
        }
    }
    return (false);
}

bool Writer_t::hasExitStatement(const State_Id_t stateId)
{
    if (0u < reader->getDeclCount(stateId, Declaration_Exit))
    {
        return (true);
    }
    else
    {
        for (size_t j = 0; j < reader->getTransitionCountFromStateId(stateId); j++)
        {
            if (reader->getTransitionFrom(stateId, j)->event.isTimeEvent)
            {
                return (true);
            }
        }
    }
    return (false);
}


std::string Writer_t::getTraceCall_entry(const State_t* state)
{
    return (this->styler->getTraceEntry() + "(" + this->styler->getStateName(state) + ");");
}

std::string Writer_t::getTraceCall_exit(const State_t* state)
{
    return (this->styler->getTraceExit() + "(" + this->styler->getStateName(state) + ");");
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
        for (size_t i = 0; i < reader->getStateCount(); i++)
        {
            State_t* tmp = reader->getState(i);
            if ((in->id != tmp->id) && (in->id == tmp->parent) && (0 == tmp->name.compare("initial")))
            {
                // a child state was found that is an initial state. Get transition
                // from this initial state, it should be one and only one.
                Transition_t* tr = reader->getTransitionFrom(tmp->id, 0);
                if (NULL == tr)
                {
                    this->errorReport("Initial state in [" + this->styler->getStateName(in) + "] as no transitions.", __LINE__);
                }
                else
                {
                    // get the transition
                    tmp = reader->getStateById(tr->stB);
                    if (NULL == tmp)
                    {
                        this->errorReport("Initial state in [" + this->styler->getStateName(in) + "] has no target.", __LINE__);
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
        for (size_t i = 0; i < reader->getStateCount(); i++)
        {
            State_t* tmp = reader->getState(i);
            if ((in->id != tmp->id) && (in->parent == tmp->id) && (0 != tmp->name.compare("initial")))
            {
                // a parent state was found that is not an initial state. Check for
                // any outgoing transitions from this state to a final state. Store
                // the id of the tmp state, since we will reuse this pointer.
                const State_Id_t tmpId = tmp->id;
                for (size_t j = 0; j < reader->getTransitionCountFromStateId(tmpId); j++)
                {
                    Transition_t* tr = reader->getTransitionFrom(tmp->id, j);
                    tmp = reader->getStateById(tr->stB);
                    if ((NULL != tmp) && (0 == tmp->name.compare("final")))
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

std::vector<State_t*> Writer_t::findInitState(void)
{
    std::vector<State_t*> states;

    for (size_t i = 0; i < reader->getStateCount(); i++)
    {
        State_t* state = reader->getState(i);
        if (0 == state->name.compare("initial"))
        {
            // check if this is top initial state
            if (0 == state->parent)
            {
                // this is the top initial, find transition from this idle
                Transition_t* tr = reader->getTransitionFrom(state->id, 0);
                if (NULL == tr)
                {
                    this->errorReport("No transition from initial state", __LINE__);
                }
                else
                {
                    // check target state (from top)
                    State_t* trStB = reader->getStateById(tr->stB);
                    if (NULL == trStB)
                    {
                        this->errorReport("Transition to null state", __LINE__);
                    }
                    else
                    {
                        // get state where it stops.
                        states = this->findEntryState(trStB);
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
