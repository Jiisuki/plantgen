
#include <string>
#include <iostream>

#include <helper.hpp>
#include <reader.hpp>

#define USE_SIMPLE_NAMES

void errorReport(const Error_t severity, std::string str, unsigned int line)
{
    switch (severity)
    {
        case Error:
            std::cout << "ERROR: ";
            break;

        case Warning:
            std::cout << "WARNING: ";
            break;

        default:
            break;
    }

    std::string fname = __FILE__;
    const size_t last_slash = fname.find_last_of("/");
    std::cout << str << " - " << fname.substr(last_slash+1) << ": " << line << std::endl;
}

std::string getStateBaseDecl(Reader_t* reader, const State_t* state)
{
    std::string decl_base = "";

#ifndef USE_SIMPLE_NAMES
    // parent nesting
    State_t* parent = reader->getStateById(state->parent);
    if (NULL != parent)
    {
        decl_base = getStateBaseDecl(reader, parent) + "_";
    }
#else
    (void) reader;
#endif

    decl_base += state->name;

    return (decl_base);
}

std::string getStateRunCycle(Reader_t* reader, const std::string modelName, const State_t* state)
{
    std::string str = modelName + "_" + getStateBaseDecl(reader, state) + "_runCycle";
    return (str);
}

std::string getStateReact(Reader_t* reader, const std::string modelName, const State_t* state)
{
    std::string str = modelName + "_" + getStateBaseDecl(reader, state) + "_react";
    return (str);
}

std::string getStateEntry(Reader_t* reader, const std::string modelName, const State_t* state)
{
    std::string str = modelName + "_" + getStateBaseDecl(reader, state) + "_entryAction";
    return (str);
}

std::string getStateExit(Reader_t* reader, const std::string modelName, const State_t* state)
{
    std::string str = modelName + "_" + getStateBaseDecl(reader, state) + "_exitAction";
    return (str);
}

std::string getStateOnCycle(Reader_t* reader, const std::string modelName, const State_t* state)
{
    std::string str = modelName + "_" + getStateBaseDecl(reader, state) + "_onCycle";
    return (str);
}

std::string getStateName(Reader_t* reader, const std::string modelName, const State_t* state)
{
    std::string str = modelName + "_State_" + getStateBaseDecl(reader, state);
    return (str);
}

std::string getEventRaise(Reader_t* reader, const std::string modelName, const Event_t* event)
{
    (void) reader;
    std::string str = modelName + "_raise" + event->name;
    return (str);
}

std::string getHandleName(const std::string modelName)
{
    std::string str = modelName + "_Handle_t";
    return (str);
}

std::string getTraceCall_entry(const std::string modelName)
{
    std::string str = modelName + "_traceEntry(handle->state);";
    return (str);
}

std::string getTraceCall_exit(const std::string modelName)
{
    std::string str = modelName + "_traceExit(handle->state);";
    return (str);
}

std::vector<State_t*> findEntryState(Reader_t* reader, const std::string modelName, State_t* in)
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
                    ERROR_REPORT(Error, "Initial state in [" + getStateName(reader, modelName, in) + "] as no transitions.");
                }
                else
                {
                    // get the transition
                    tmp = reader->getStateById(tr->stB);
                    if (NULL == tmp)
                    {
                        ERROR_REPORT(Error, "Initial state in [" + getStateName(reader, modelName, in) + "] has null target.");
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

std::vector<State_t*> findFinalState(Reader_t* reader, const std::string modelName, State_t* in)
{
    // this function will check if the in state has an outgoing transition to
    // a final state and follow the path until a state is reached that does not
    // contain an initial sub-state.
    (void) modelName;

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

std::vector<State_t*> findInitState(Reader_t* reader, const size_t nStates, const std::string modelName)
{
    std::vector<State_t*> states;

    for (size_t i = 0; i < nStates; i++)
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
                    ERROR_REPORT(Error, "No transition from initial state");
                }
                else
                {
                    // check target state (from top)
                    State_t* trStB = reader->getStateById(tr->stB);
                    if (NULL == trStB)
                    {
                        ERROR_REPORT(Error, "Transition to null state");
                    }
                    else
                    {
                        // get state where it stops.
                        states = findEntryState(reader, modelName, trStB);
                    }
                }
            }
        }
    }

    return (states);
}

std::string getIndent(const size_t level)
{
    std::string s = "";
    for (size_t i = 0; i < level; i++)
    {
        s += "    ";
    }
    return (s);
}

std::string getIfElseIf(const size_t i)
{
    std::string s = "if";
    if (0 != i)
    {
        s = "else if";
    }
    return (s);
}

