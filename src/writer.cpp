#include <iostream>
#include <fstream>
#include <reader.hpp>
#include <writer.hpp>
#include <helper.hpp>

bool doTracing = false;

void Writer_setTracing(const bool setting)
{
    doTracing = setting;
}

void writePrototype_runCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if ((0 != state->name.compare("initial")) && (0 != state->name.compare("final")))
        {
            writer->out_c << getIndent(0) << "static void " << getStateRunCycle(reader, modelName, state) << "(" << getHandleName(modelName) << " handle);" << std::endl;
        }
    }
    writer->out_c << std::endl;
}

void writePrototype_react(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if ((0 != state->name.compare("initial")) && (0 != state->name.compare("final")))
        {
            writer->out_c << getIndent(0) << "static bool " << getStateReact(reader, modelName, state) << "(" << getHandleName(modelName) << " handle);" << std::endl;
        }
    }
    writer->out_c << std::endl;
}

void writePrototype_entryAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        const size_t numDecl = reader->getDeclCount(state->id, Declaration_Entry);
        bool writePrototype = (0 != numDecl);

        if ((0 != state->name.compare("initial")) && (writePrototype))
        {
            writer->out_c << getIndent(0) << "static void " << getStateEntry(reader, modelName, state) << "(" << getHandleName(modelName) << " handle);" << std::endl;
        }
    }
    writer->out_c << std::endl;
}

void writePrototype_exitAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        const size_t numDecl = reader->getDeclCount(state->id, Declaration_Exit);
        bool writePrototype = (0 != numDecl);

        if ((0 != state->name.compare("initial")) && (writePrototype))
        {
            writer->out_c << getIndent(0) << "static void " << getStateExit(reader, modelName, state) << "(" << getHandleName(modelName) << " handle);" << std::endl;
        }
    }
    writer->out_c << std::endl;
}

void writePrototype_traceEntry(Writer_t* writer, const std::string modelName)
{
    writer->out_h << "extern void " << modelName << "_traceEntry(const " << modelName << "_State_t state);" << std::endl;
}

void writePrototype_traceExit(Writer_t* writer, const std::string modelName)
{
    writer->out_h << "extern void " << modelName << "_traceExit(const " << modelName << "_State_t state);" << std::endl;
}

void writeImplementation_init(Writer_t* writer, Reader_t* reader, const State_t* firstState, const std::string modelName)
{
    writer->out_c << "void " << modelName << "_init(" << getHandleName(modelName) << " handle)" << std::endl;
    writer->out_c << "{" << std::endl;
    writer->out_c << getIndent(1) << "/* Clear events. */" << std::endl;
    writer->out_c << getIndent(1) << "clearInEvents(handle);" << std::endl;
    if (NULL != firstState)
    {
        writer->out_c << std::endl;
        writer->out_c << getIndent(1) << "/* Set initial state. */" << std::endl;
        writer->out_c << getIndent(1) << "handle->state = " << getStateName(reader, modelName, firstState) << ";" << std::endl;
        if (doTracing)
        {
            writer->out_c << getIndent(1) << getTraceCall_entry(modelName) << std::endl;
        }
    }
    writer->out_c << "}" << std::endl << std::endl;
}

void writeImplementation_raiseEvent(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName)
{
    for (size_t i = 0; i < nEvents; i++)
    {
        Event_t* ev = reader->getEvent(i);
        writer->out_c << getIndent(0) << "void " << getEventRaise(reader, modelName, ev) << "(" << getHandleName(modelName) << " handle)" << std::endl;
        writer->out_c << getIndent(0) << "{" << std::endl;
        writer->out_c << getIndent(1) << "handle->events." << ev->name << " = true;" << std::endl;
        writer->out_c << getIndent(1) << "runCycle(handle);" << std::endl;
        writer->out_c << getIndent(0) << "}" << std::endl << std::endl;
    }
}

void writeImplementation_clearInEvents(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName)
{
    (void) modelName;
    writer->out_c << "static void clearInEvents(" << getHandleName(modelName) << " handle)" << std::endl;
    writer->out_c << "{" << std::endl;
    for (size_t i = 0; i < nEvents; i++)
    {
        Event_t* ev = reader->getEvent(i);
        writer->out_c << getIndent(1) << "handle->events." << ev->name << " = false;" << std::endl;
    }
    writer->out_c << "}" << std::endl << std::endl;
}

void writeImplementation_isFinal(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    writer->out_c << "void " << modelName << "_isFinal(" << getHandleName(modelName) << " handle)" << std::endl;
    writer->out_c << "{" << std::endl;
    writer->out_c << getIndent(1) << "// TODO" << std::endl;
    writer->out_c << "}" << std::endl << std::endl;
}

void writeImplementation_topRunCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    size_t writeNumber = 0;
    writer->out_c << "static void runCycle(" << getHandleName(modelName) << " handle)" << std::endl;
    writer->out_c << "{" << std::endl;
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if ((0 != state->name.compare("initial")) && (0 != state->name.compare("final")))
        {
            writer->out_c << getIndent(1) << getIfElseIf(writeNumber) << " (handle->state == " << getStateName(reader, modelName, state) << ")" << std::endl;
            writer->out_c << getIndent(1) << "{" << std::endl;
            writer->out_c << getIndent(2) << getStateRunCycle(reader, modelName, state) << "(handle);" << std::endl;
            writer->out_c << getIndent(1) << "}" << std::endl;
            writeNumber++;
        }
    }
    if (0 < nStates)
    {
        writer->out_c << getIndent(1) << "else" << std::endl;
        writer->out_c << getIndent(1) << "{" << std::endl;
        writer->out_c << getIndent(2) << "// possibly final state!" << std::endl;
        writer->out_c << getIndent(1) << "}" << std::endl << std::endl;
    }
    writer->out_c << getIndent(1) << "clearInEvents(handle);" << std::endl;
    writer->out_c << "}" << std::endl << std::endl;
}

void writeImplementation_runCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const bool isParentFirstExec, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
        {
            // initial or final state, no runcycle
        }
        else
        {
            bool isEmptyBody = true;
            writer->out_c << "static void " << getStateRunCycle(reader, modelName, state) << "(" << getHandleName(modelName) << " handle)" << std::endl;
            writer->out_c << "{" << std::endl;

            const size_t nOutTr = reader->getTransitionCountFromStateId(state->id);

            size_t baseIndent = 1;
            bool isParentState = false;

            // write parent react
            State_t* parentState = reader->getStateById(state->parent);
            if (NULL != parentState)
            {
                isEmptyBody = false;
                isParentState = true;
                writer->out_c << getIndent(1) << "if (true == " << getStateReact(reader, modelName, parentState) << "(handle))" << std::endl;
                writer->out_c << getIndent(1) << "{" << std::endl;
                // if tracing is enabled
                if (doTracing)
                {
                    writer->out_c << getIndent(2) << getTraceCall_exit(modelName) << std::endl;
                }
                // if exit actions exists
                if (0 < reader->getDeclCount(state->id, Declaration_Exit))
                {
                    writer->out_c << getIndent(2) << getStateExit(reader, modelName, state) << "(handle);" << std::endl;
                }
                writer->out_c << getIndent(2) << getStateRunCycle(reader, modelName, parentState) << "(handle);" << std::endl;
                writer->out_c << getIndent(1) << "}" << std::endl;
                if (0 < nOutTr)
                {
                    writer->out_c << getIndent(1) << "else" << std::endl;
                    writer->out_c << getIndent(1) << "{" << std::endl;
                    baseIndent = 2;
                }
            }

            if (0 < nOutTr)
            {
                for (size_t j = 0; j < nOutTr; j++)
                {
                    Transition_t* tr = reader->getTransitionFrom(state->id, j);
                    if (0 == tr->event.name.compare("null"))
                    {
                        State_t* trStB = reader->getStateById(tr->stB);
                        if (NULL == trStB)
                        {
                            ERROR_REPORT(Warning, "Null transition detected");
                        }
                        else if (0 != trStB->name.compare("final"))
                        {
                            ERROR_REPORT(Warning, "Null transition " +
                                    getStateBaseDecl(reader, state) + " -> " +
                                    getStateBaseDecl(reader, trStB));

                            // handle as a oncycle transition?
                            isEmptyBody = false;
                            writer->out_c << getIndent(baseIndent + 0) << getIfElseIf(j) << " (true)" << std::endl;
                            writer->out_c << getIndent(baseIndent + 0) << "{" << std::endl;
                            // if tracing is enabled
                            if (doTracing)
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getTraceCall_exit(modelName) << std::endl;
                            }
                            // is exit function exists
                            if (0 < reader->getDeclCount(state->id, Declaration_Exit))
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getStateExit(reader, modelName, state) << "(handle);" << std::endl;
                            }
                            // check target state
                            State_t* targetState = findEntryState(reader, modelName, trStB);
                            if (0 == targetState->name.compare("final"))
                            {
                                targetState = findFinalState(reader, modelName, targetState);
                            }
                            writer->out_c << getIndent(baseIndent + 1) << "handle->state = " << getStateName(reader, modelName, targetState) << ";" << std::endl;
                            // if tracing is enabled
                            if (doTracing)
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getTraceCall_entry(modelName) << std::endl;
                            }
                            // if entry function exists
                            if (0 < reader->getDeclCount(targetState->id, Declaration_Entry))
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getStateEntry(reader, modelName, targetState) << "(handle);" << std::endl;
                            }
                            writer->out_c << getIndent(baseIndent + 0) << "}" << std::endl;
                        }
                    }
                    else
                    {
                        State_t* trStB = reader->getStateById(tr->stB);
                        if (NULL == trStB)
                        {
                            ERROR_REPORT(Warning, "Transition to null state from " + state->name);
                        }
                        else
                        {
                            isEmptyBody = false;
                            writer->out_c << getIndent(baseIndent + 0) << getIfElseIf(j) << " (handle->events." << tr->event.name << ")" << std::endl;
                            writer->out_c << getIndent(baseIndent + 0) << "{" << std::endl;
                            // if tracing is enabled
                            if (doTracing)
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getTraceCall_exit(modelName) << std::endl;
                            }
                            // if exit function exists
                            if (0 < reader->getDeclCount(state->id, Declaration_Exit))
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getStateExit(reader, modelName, state) << "(handle);" << std::endl;
                            }
                            State_t* targetState = findEntryState(reader, modelName, trStB);
                            if (0 == targetState->name.compare("final"))
                            {
                                targetState = findFinalState(reader, modelName, targetState);
                            }
                            writer->out_c << getIndent(baseIndent + 1) << "handle->state = " << getStateName(reader, modelName, targetState) << ";" << std::endl;
                            // if tracing is enabled
                            if (doTracing)
                            {
                                writer->out_c << getIndent(baseIndent + 1) << getTraceCall_entry(modelName) << std::endl;
                            }
                            // if entry function exists
                            if ((0 != targetState->name.compare("final")) && (0 < reader->getDeclCount(targetState->id, Declaration_Entry)))
                            {
                                // only entry on states, not final.
                                writer->out_c << getIndent(baseIndent + 1) << getStateEntry(reader, modelName, targetState) << "(handle);" << std::endl;
                            }
                            writer->out_c << getIndent(baseIndent + 0) << "}" << std::endl;
                        }
                    }
                }
                if (isParentFirstExec && isParentState)
                {
                    writer->out_c << getIndent(1) << "}" << std::endl;
                }
            }

            if (isEmptyBody)
            {
                writer->out_c << getIndent(1) << "(void) handle;" << std::endl;
            }

            writer->out_c << "}" << std::endl << std::endl;
        }
    }
}

void writeImplementation_reactions(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    // TODO: This has to nest as well to handle all parent states.

    // implement the react functions, pretty much the same as above but just
    // flag if the event is to be taken or not.
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
        {
            // No reaction on initial or final states.
        }
        else
        {
            writer->out_c << getIndent(0) << "static bool " << getStateReact(reader, modelName, state) << "(" << getHandleName(modelName) << " handle)" << std::endl;
            writer->out_c << getIndent(0) << "{" << std::endl;

            bool isEmptyBody = true;
            bool isParentState = false;
            size_t baseIndent = 1;

            const size_t nOutTr = reader->getTransitionCountFromStateId(state->id);

            // write parent react
            State_t* parentState = reader->getStateById(state->parent);
            if (NULL != parentState)
            {
                if (isEmptyBody)
                {
                    writer->out_c << getIndent(1) << "bool hasReacted = false;" << std::endl;
                }
                isEmptyBody = false;
                isParentState = true;
                writer->out_c << getIndent(1) << "if (true == " << getStateReact(reader, modelName, parentState) << "(handle))" << std::endl;
                writer->out_c << getIndent(1) << "{" << std::endl;
                // if exit actions exists
                writer->out_c << getIndent(2) << "hasReacted = true;" << std::endl;
                writer->out_c << getIndent(1) << "}" << std::endl;
                if (0 < nOutTr)
                {
                    writer->out_c << getIndent(1) << "else" << std::endl;
                    writer->out_c << getIndent(1) << "{" << std::endl;
                    baseIndent = 2;
                }
            }

            if (0 < nOutTr)
            {
                for (size_t j = 0; j < nOutTr; j++)
                {
                    Transition_t* tr = reader->getTransitionFrom(state->id, j);
                    if (0 == tr->event.name.compare("null"))
                    {
                        State_t* trStB = reader->getStateById(tr->stB);
                        if (NULL == trStB)
                        {
                            ERROR_REPORT(Warning, "Null transition detected");
                        }
                        else if (0 != trStB->name.compare("final"))
                        {
                            ERROR_REPORT(Warning, "Null transition " +
                                    getStateBaseDecl(reader, state) + " -> " +
                                    getStateBaseDecl(reader, trStB));

                            // handle as a oncycle transition, not pretty.
                            if (isEmptyBody)
                            {
                                writer->out_c << getIndent(1) << "bool hasReacted = false;" << std::endl;
                            }
                            isEmptyBody = false;
                            writer->out_c << getIndent(baseIndent + 0) << getIfElseIf(j) << " (true)" << std::endl;
                            writer->out_c << getIndent(baseIndent + 0) << "{" << std::endl;
                            writer->out_c << getIndent(baseIndent + 1) << "hasReacted = true;" << std::endl;
                            writer->out_c << getIndent(baseIndent + 0) << "}" << std::endl;
                        }
                        else
                        {
                            /* This will be handled on outgoing to final */
                        }
                    }
                    else
                    {
                        State_t* trStB = reader->getStateById(tr->stB);
                        if (NULL == trStB)
                        {
                            ERROR_REPORT(Warning, "Transition to null state from " + state->name);
                        }
                        else
                        {
                            if (isEmptyBody)
                            {
                                writer->out_c << getIndent(1) << "bool hasReacted = false;" << std::endl;
                            }
                            isEmptyBody = false;
                            writer->out_c << getIndent(baseIndent + 0) << getIfElseIf(j) << " (handle->events." << tr->event.name << ")" << std::endl;
                            writer->out_c << getIndent(baseIndent + 0) << "{" << std::endl;
                            writer->out_c << getIndent(baseIndent + 1) << "hasReacted = true;" << std::endl;
                            writer->out_c << getIndent(baseIndent + 0) << "}" << std::endl;
                        }
                    }
                }
                if (isParentState)
                {
                    writer->out_c << getIndent(1) << "}" << std::endl;
                }
            }

            if (isEmptyBody)
            {
                writer->out_c << getIndent(1) << "(void) handle;" << std::endl;
                writer->out_c << getIndent(1) << "return (false);" << std::endl;
            }
            else
            {
                writer->out_c << getIndent(1) << "return (hasReacted);" << std::endl;
            }

            writer->out_c << getIndent(0) << "}" << std::endl << std::endl;
        }
    }
}

void writeImplementation_entryAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader->getDeclCount(state->id, Declaration_Entry);
            if (0 < numDecl)
            {
                writer->out_c << getIndent(0) << "static void " << getStateEntry(reader, modelName, state) << "(" << getHandleName(modelName) << " handle)" << std::endl;
                writer->out_c << getIndent(0) << "{" << std::endl;
                for (size_t j = 0; j < numDecl; j++)
                {
                    State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_Entry, j);
                    if (Declaration_Entry == decl->type)
                    {
                        writer->out_c << getIndent(1) << decl->declaration << std::endl;
                    }
                }
                writer->out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
    writer->out_c << std::endl;
}

void writeImplementation_exitAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader->getDeclCount(state->id, Declaration_Exit);
            if (0 < numDecl)
            {
                writer->out_c << getIndent(0) << "static void " << getStateExit(reader, modelName, state) << "(" << getHandleName(modelName) << " handle)" << std::endl;
                writer->out_c << getIndent(0) << "{" << std::endl;
                for (size_t j = 0; j < numDecl; j++)
                {
                    State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_Exit, j);
                    if (Declaration_Exit == decl->type)
                    {
                        writer->out_c << getIndent(1) << decl->declaration << std::endl;
                    }
                }
                writer->out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
    writer->out_c << std::endl;
}

void writeImplementation_oncycleAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if (0 != state->name.compare("initial"))
        {
            const size_t numDecl = reader->getDeclCount(state->id, Declaration_OnCycle);
            if (0 < numDecl)
            {
                writer->out_c << getIndent(0) << "static void " << getStateOnCycle(reader, modelName, state) << "(" << getHandleName(modelName) << " handle)" << std::endl;
                writer->out_c << getIndent(0) << "{" << std::endl;
                for (size_t j = 0; j < numDecl; j++)
                {
                    State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_OnCycle, j);
                    if (Declaration_OnCycle == decl->type)
                    {
                        writer->out_c << getIndent(1) << decl->declaration << std::endl;
                    }
                }
                writer->out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
    writer->out_c << std::endl;
}


