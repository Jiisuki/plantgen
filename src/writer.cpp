#include <iostream>
#include <fstream>
#include <sstream>
#include <reader.hpp>
#include <writer.hpp>
#include <helper.hpp>

bool doTracing = false;
static std::vector<std::string> tokenize(std::string str);
static void parseDeclaration(Writer_t* writer, Reader_t* reader, const std::string declaration);
static std::string parseGuard(Writer_t* writer, Reader_t* reader, const std::string guardStrRaw);
static void parseChoicePath(Writer_t* writer, Reader_t* reader, const std::string modelName, State_t* initialChoice, size_t indentLevel);

void Writer_setTracing(const bool setting)
{
    doTracing = setting;
}

void writePrototype_runCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    for (size_t i = 0; i < nStates; i++)
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
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
        {
            // no react for initial or final states.
        }
        else if (state->isChoice)
        {
            // no react for choice states.
        }
        else
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

void writePrototype_raiseInEvent(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName)
{
    for (size_t i = 0; i < nEvents; i++)
    {
        Event_t* ev = reader->getInEvent(i);
        writer->out_h << getIndent(0) << "void " << getEventRaise(reader, modelName, ev) << "(" << getHandleName(modelName) << " handle";
        if (ev->requireParameter)
        {
            writer->out_h << ", const " << ev->parameterType << " param";
        }
        writer->out_h << ");" << std::endl;
    }
}

void writePrototype_traceEntry(Writer_t* writer, const std::string modelName)
{
    writer->out_h << "extern void " << modelName << "_traceEntry(const " << modelName << "_State_t state);" << std::endl;
}

void writePrototype_traceExit(Writer_t* writer, const std::string modelName)
{
    writer->out_h << "extern void " << modelName << "_traceExit(const " << modelName << "_State_t state);" << std::endl;
}

void writeDeclaration_stateList(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    writer->out_h << getIndent(0) << "typedef enum" << std::endl;
    writer->out_h << getIndent(0) << "{" << std::endl;
    writer->out_h << getIndent(1) << modelName << "_State_Final = 0," << std::endl;
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if (NULL != state)
        {
            if ((0 != state->name.compare("initial")) && (0 != state->name.compare("final")) && (!state->isChoice))
            {
                writer->out_h << getIndent(1) << getStateName(reader, modelName, state) << "," << std::endl;
            }
        }
    }
    writer->out_h << getIndent(0) << "} " << modelName << "_State_t;" << std::endl << std::endl;
}

void writeDeclaration_eventList(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName)
{
    writer->out_h << getIndent(0) << "typedef struct" << std::endl;
    writer->out_h << getIndent(0) << "{" << std::endl;
    for (size_t i = 0; i < nEvents; i++)
    {
        Event_t* ev = reader->getInEvent(i);
        writer->out_h << getIndent(1) << "struct " << modelName << "_Event_" << ev->name << "_t" << std::endl;
        writer->out_h << getIndent(1) << "{" << std::endl;
        writer->out_h << getIndent(2) << "bool isRaised;" << std::endl;
        if (ev->requireParameter)
        {
            writer->out_h << getIndent(2) << ev->parameterType << " param;" << std::endl;
        }
        writer->out_h << getIndent(1) << "} " << ev->name << ";" << std::endl;
    }
    writer->out_h << getIndent(0) << "} " << modelName << "_EventList_t;" << std::endl;
    writer->out_h << std::endl;
}

void writeDeclaration_variableList(Writer_t* writer, Reader_t* reader, const size_t nVariables, const std::string modelName)
{
    writer->out_h << getIndent(0) << "typedef struct" << std::endl;
    writer->out_h << getIndent(0) << "{" << std::endl;

    // write private
    writer->out_h << getIndent(1) << "struct " << modelName << "_PrivateVariables_t" << std::endl;
    writer->out_h << getIndent(1) << "{" << std::endl;
    for (size_t i = 0; i < nVariables; i++)
    {
        Variable_t* var = reader->getVariable(i);
        if (var->isPrivate)
        {
            writer->out_h << getIndent(2) << var->type << " " << var->name << ";" << std::endl;
        }
    }
    writer->out_h << getIndent(1) << "} private;" << std::endl;

    // write public
    writer->out_h << getIndent(1) << "struct " << modelName << "_PublicVariables_t" << std::endl;
    writer->out_h << getIndent(1) << "{" << std::endl;
    for (size_t i = 0; i < nVariables; i++)
    {
        Variable_t* var = reader->getVariable(i);
        if (true != var->isPrivate)
        {
            writer->out_h << getIndent(2) << var->type << " " << var->name << ";" << std::endl;
        }
    }
    writer->out_h << getIndent(1) << "} public;" << std::endl;
    writer->out_h << getIndent(0) << "} " << modelName << "_Variables_t;" << std::endl << std::endl;
}

void writeDeclaration_stateMachine(Writer_t* writer, Reader_t* reader, const std::string modelName)
{
    // write internal structure
    writer->out_h << getIndent(0) << "typedef struct" << std::endl;
    writer->out_h << getIndent(0) << "{" << std::endl;
    writer->out_h << getIndent(1) << modelName << "_State_t state;" << std::endl;
    writer->out_h << getIndent(1) << modelName << "_EventList_t events;" << std::endl;
    writer->out_h << getIndent(1) << modelName << "_Variables_t variables;" << std::endl;
    writer->out_h << getIndent(0) << "} " << modelName << "_t;" << std::endl << std::endl;
}

void writeImplementation_init(Writer_t* writer, Reader_t* reader, const std::vector<State_t*> firstState, const std::string modelName)
{
    writer->out_c << "void " << modelName << "_init(" << getHandleName(modelName) << " handle)" << std::endl;
    writer->out_c << "{" << std::endl;

    // write variable inits
    writer->out_c << getIndent(1) << "/* Initialise variables. */" << std::endl;
    for (size_t i = 0; i < reader->getVariableCount(); i++)
    {
        Variable_t* var = reader->getVariable(i);
        if (var->isPrivate)
        {
            writer->out_c << getIndent(1) << "handle->variables.private.";
        }
        else
        {
            writer->out_c << getIndent(1) << "handle->variables.public.";
        }
        writer->out_c << var->name << " = ";
        if (var->specificInitialValue)
        {
            writer->out_c << var->initialValue << ";" << std::endl;
        }
        else
        {
            writer->out_c << "0;" << std::endl;
        }
    }
    writer->out_c << std::endl;

    // write event clearing
    writer->out_c << getIndent(1) << "/* Clear events. */" << std::endl;
    writer->out_c << getIndent(1) << "clearInEvents(handle);" << std::endl;
    if (0 < firstState.size())
    {
        writer->out_c << std::endl;
        writer->out_c << getIndent(1) << "/* Set initial state. */" << std::endl;
        State_t* targetState = NULL;
        for (size_t i = 0; i < firstState.size(); i++)
        {
            targetState = firstState[i];
            const size_t numDecl = reader->getDeclCount(targetState->id, Declaration_Entry);
            if (0 < numDecl)
            {
                // write entry call
                writer->out_c << getIndent(1) << getStateEntry(reader, modelName, targetState) << "(handle);" << std::endl;
            }
        }
        writer->out_c << getIndent(1) << "handle->state = " << getStateName(reader, modelName, targetState) << ";" << std::endl;
        if (doTracing)
        {
            writer->out_c << getIndent(1) << getTraceCall_entry(modelName) << std::endl;
        }
    }
    writer->out_c << "}" << std::endl << std::endl;
}

void writeImplementation_raiseInEvent(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName)
{
    for (size_t i = 0; i < nEvents; i++)
    {
        Event_t* ev = reader->getInEvent(i);
        writer->out_c << getIndent(0) << "void " << getEventRaise(reader, modelName, ev) << "(" << getHandleName(modelName) << " handle";
        if (ev->requireParameter)
        {
            writer->out_c << ", const " << ev->parameterType << " param";
        }
        writer->out_c << ")" << std::endl;
        writer->out_c << getIndent(0) << "{" << std::endl;
        if (ev->requireParameter)
        {
            writer->out_c << getIndent(1) << "handle->events." << ev->name << ".param = param;" << std::endl;
        }
        writer->out_c << getIndent(1) << "handle->events." << ev->name << ".isRaised = true;" << std::endl;
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
        Event_t* ev = reader->getInEvent(i);
        writer->out_c << getIndent(1) << "handle->events." << ev->name << ".isRaised = false;" << std::endl;
    }
    writer->out_c << "}" << std::endl << std::endl;
}

void writeImplementation_isFinal(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName)
{
    (void) reader;
    (void) nStates;
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
        else if (state->isChoice)
        {
            // no run cycle for choice
        }
        else
        {
            bool isEmptyBody = true;
            writer->out_c << "static void " << getStateRunCycle(reader, modelName, state) << "(" << getHandleName(modelName) << " handle)" << std::endl;
            writer->out_c << "{" << std::endl;

            // write comment declaration here if one exists.
            const size_t numCommentLines = reader->getDeclCount(state->id, Declaration_Comment);
            if (0 < numCommentLines)
            {
                isEmptyBody = false;
                for (size_t j = 0; j < numCommentLines; j++)
                {
                    State_Declaration_t* decl = reader->getDeclFromStateId(state->id, Declaration_Comment, j);
                    writer->out_c << getIndent(1) << "/* " << decl->declaration << " */" << std::endl;
                }
                writer->out_c << std::endl;
            }

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
#if 0
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
#endif
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

                            if (tr->hasGuard)
                            {
                                std::string guardStr = parseGuard(writer, reader, tr->guard);
                                writer->out_c << getIndent(baseIndent + 0)
                                              << getIfElseIf(j)
                                              << " ((handle->events."
                                              << tr->event.name
                                              << ".isRaised) && ("
                                              << guardStr
                                              << "))"
                                              << std::endl;
                            }
                            else
                            {
                                writer->out_c << getIndent(baseIndent + 0) << getIfElseIf(j) << " (handle->events." << tr->event.name << ".isRaised)" << std::endl;
                            }
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
                            // TODO: do entry actins on all states entered
                            // towards the goal! Might needs some work..
                            std::vector<State_t*> enteredStates = findEntryState(reader, modelName, trStB);
                            State_t* finalState = NULL;
                            for (size_t j = 0; j < enteredStates.size(); j++)
                            {
                                finalState = enteredStates[j];

                                // if tracing is enabled, TODO fix function
                                // call so send state name.
                                if (doTracing)
                                {
                                    writer->out_c << getIndent(baseIndent + 1) << getTraceCall_entry(modelName) << std::endl;
                                }

                                if (0 < reader->getDeclCount(finalState->id, Declaration_Entry))
                                {
                                    writer->out_c << getIndent(baseIndent + 1) << getStateEntry(reader, modelName, finalState) << "(handle);" << std::endl;
                                }
                            }
                            // handle choice node?
                            if ((NULL != finalState) && (finalState->isChoice))
                            {
                                parseChoicePath(writer, reader, modelName, finalState, baseIndent + 1);
                            }
                            else
                            {
                                writer->out_c << getIndent(baseIndent + 1) << "handle->state = " << getStateName(reader, modelName, finalState) << ";" << std::endl;
                            }

                            // TODO: handle super-step to final
#if 0
                            if (0 == targetState->name.compare("final"))
                            {
                                targetState = findFinalState(reader, modelName, targetState);
                            }
                            writer->out_c << getIndent(baseIndent + 1) << "handle->state = " << getStateName(reader, modelName, targetState) << ";" << std::endl;
                            // if entry function exists
                            if ((0 != targetState->name.compare("final")) && (0 < reader->getDeclCount(targetState->id, Declaration_Entry)))
                            {
                                // only entry on states, not final.
                                writer->out_c << getIndent(baseIndent + 1) << getStateEntry(reader, modelName, targetState) << "(handle);" << std::endl;
                            }
#endif
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
    // implement the react functions, pretty much the same as above but just
    // flag if the event is to be taken or not.
    for (size_t i = 0; i < nStates; i++)
    {
        State_t* state = reader->getState(i);
        if ((0 == state->name.compare("initial")) || (0 == state->name.compare("final")))
        {
            // No reaction on initial or final states.
        }
        else if (state->isChoice)
        {
            // no reaction on choice states.
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

                            if (tr->hasGuard)
                            {
                                std::string guardStr = parseGuard(writer, reader, tr->guard);
                                writer->out_c << getIndent(baseIndent + 0)
                                              << getIfElseIf(j)
                                              << " ((handle->events."
                                              << tr->event.name
                                              << ".isRaised) && ("
                                              << guardStr
                                              << "))"
                                              << std::endl;
                            }
                            else
                            {
                                writer->out_c << getIndent(baseIndent + 0) << getIfElseIf(j) << " (handle->events." << tr->event.name << ".isRaised)" << std::endl;
                            }

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
                        parseDeclaration(writer, reader, decl->declaration);
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
                        parseDeclaration(writer, reader, decl->declaration);
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
                        parseDeclaration(writer, reader, decl->declaration);
                    }
                }
                writer->out_c << getIndent(0) << "}" << std::endl << std::endl;
            }
        }
    }
    writer->out_c << std::endl;
}

static std::vector<std::string> tokenize(std::string str)
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

static void parseDeclaration(Writer_t* writer, Reader_t* reader, const std::string declaration)
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
                            wstr += "handle->events." + ev->name + ".param";
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

    if (';' != wstr.back())
    {
        wstr += ";";
    }

    writer->out_c << getIndent(1) << wstr << std::endl;
}

static std::string parseGuard(Writer_t* writer, Reader_t* reader, const std::string guardStrRaw)
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
                            wstr += "handle->events." + ev->name + ".param";
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

static void parseChoicePath(Writer_t* writer, Reader_t* reader, const std::string modelName, State_t* state, size_t indentLevel)
{
    // check all transitions from the choice..
    writer->out_c << std::endl << getIndent(indentLevel) << "/* Choice: " << state->name << " */" << std::endl;

    const size_t numChoiceTr = reader->getTransitionCountFromStateId(state->id);
    if (numChoiceTr < 2)
    {
        ERROR_REPORT(Error, "Only one transition from choice " + state->name);
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
                writer->out_c << getIndent(indentLevel + 0) << getIfElseIf(k++) << " (" << parseGuard(writer, reader, tr->guard) << ")" << std::endl;
                writer->out_c << getIndent(indentLevel + 0) << "{" << std::endl;
                State_t* guardedState = reader->getStateById(tr->stB);
                writer->out_c << getIndent(indentLevel + 1) << "// goto: " << guardedState->name << std::endl;
                if (NULL == guardedState)
                {
                    ERROR_REPORT(Error, "Invalid transition from choice " + state->name);
                }
                else
                {
                    std::vector<State_t*> enteredStates = findEntryState(reader, modelName, guardedState);
                    State_t* finalState = NULL;
                    for (size_t j = 0; j < enteredStates.size(); j++)
                    {
                        finalState = enteredStates[j];

                        // TODO: fix call so send state name.
                        if (doTracing)
                        {
                            writer->out_c << getIndent(indentLevel + 1) << getTraceCall_entry(modelName) << std::endl;
                        }
                        if (0 < reader->getDeclCount(finalState->id, Declaration_Entry))
                        {
                            writer->out_c << getIndent(indentLevel + 1) << getStateEntry(reader, modelName, finalState) << "(handle);" << std::endl;
                        }
                    }
                    if (NULL != finalState)
                    {
                        if (finalState->isChoice)
                        {
                            // nest ..
                            parseChoicePath(writer, reader, modelName, finalState, indentLevel + 1);
                        }
                        else
                        {
                            writer->out_c << getIndent(indentLevel + 1) << "handle->state = " << getStateName(reader, modelName, finalState) << ";" << std::endl;
                        }
                    }
                }
                writer->out_c << getIndent(indentLevel + 0) << "}" << std::endl;
            }
        }
        if (NULL != defaultTr)
        {
            // write default transition.
            writer->out_c << getIndent(indentLevel + 0) << "else" << std::endl;
            writer->out_c << getIndent(indentLevel + 0) << "{" << std::endl;
            State_t* guardedState = reader->getStateById(defaultTr->stB);
            writer->out_c << getIndent(indentLevel + 1) << "// goto: " << guardedState->name << std::endl;
            if (NULL == guardedState)
            {
                ERROR_REPORT(Error, "Invalid transition from choice " + state->name);
            }
            else
            {
                std::vector<State_t*> enteredStates = findEntryState(reader, modelName, guardedState);
                State_t* finalState = NULL;
                for (size_t j = 0; j < enteredStates.size(); j++)
                {
                    finalState = enteredStates[j];

                    // TODO: fix call so send state name.
                    if (doTracing)
                    {
                        writer->out_c << getIndent(indentLevel + 1) << getTraceCall_entry(modelName) << std::endl;
                    }
                    if (0 < reader->getDeclCount(finalState->id, Declaration_Entry))
                    {
                        writer->out_c << getIndent(indentLevel + 1) << getStateEntry(reader, modelName, finalState) << "(handle);" << std::endl;
                    }
                }
                if (NULL != finalState)
                {
                    if (finalState->isChoice)
                    {
                        // nest ..
                        parseChoicePath(writer, reader, modelName, finalState, indentLevel + 1);
                    }
                    else
                    {
                        writer->out_c << getIndent(indentLevel + 1) << "handle->state = " << getStateName(reader, modelName, finalState) << ";" << std::endl;
                    }
                }
            }
            writer->out_c << getIndent(indentLevel + 0) << "}" << std::endl;
        }
        else
        {
            ERROR_REPORT(Error, "No default transition from " + state->name);
        }
    }
}

