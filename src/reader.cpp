#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <reader.hpp>

Reader_t::Reader_t(std::string filename)
{
    this->in.open(filename);
    if (true != this->in.is_open())
    {
        // error
    }
    else
    {
        // set default model name
        size_t dotindex = filename.find_last_of(".");
        this->modelName = filename.substr(0, dotindex);
        this->collectStates();
    }
}

Reader_t::~Reader_t()
{
    if (this->in.is_open())
    {
        this->in.close();
    }
}

// public
std::string Reader_t::getModelName(void)
{
    return (this->modelName);
}

// public
size_t Reader_t::getFunctionCount(void)
{
    return (this->functions.size());
}

// public
Function_t* Reader_t::getFunction(const size_t id)
{
    Function_t* fcn = NULL;

    if (id < this->functions.size())
    {
        fcn = &this->functions[id];
    }

    return (fcn);
}

// public
size_t Reader_t::getVariableCount(void)
{
    return (this->variables.size());
}

// public
Variable_t* Reader_t::getVariable(const size_t id)
{
    Variable_t* var = NULL;

    if (id < this->variables.size())
    {
        var = &this->variables[id];
    }

    return (var);
}

// public
size_t Reader_t::getImportCount(void)
{
    return (this->variables.size());
}

// public
Import_t* Reader_t::getImport(const size_t id)
{
    Import_t* imp = NULL;

    if (id < this->imports.size())
    {
        imp = &this->imports[id];
    }

    return (imp);
}

// public
size_t Reader_t::getStateCount(void)
{
    return (this->states.size());
}

// public
State_t* Reader_t::getState(size_t id)
{
    State_t* state = NULL;
    if (id < this->states.size())
    {
        state = &this->states[id];
    }
    return (state);
}

// public
State_t* Reader_t::getStateById(const State_Id_t id)
{
    State_t* state = NULL;
    for (size_t i = 0; i < this->states.size(); i++)
    {
        if (id == this->states[i].id)
        {
            state = &this->states[i];
            break;
        }
    }
    return (state);
}

// public
size_t Reader_t::getEventCount(void)
{
    return (this->events.size());
}

// public
Event_t* Reader_t::getEvent(size_t id)
{
    Event_t* event = NULL;
    if (id < this->events.size())
    {
        event = &this->events[id];
    }
    return (event);
}

// public
size_t Reader_t::getTransitionCountFromStateId(const State_Id_t id)
{
    size_t nTr = 0;
    for (size_t i = 0; i < this->transitions.size(); i++)
    {
        if (id == this->transitions[i].stA)
        {
            nTr++;
        }
    }
    return (nTr);
}

Transition_t* Reader_t::getTransitionFrom(const State_Id_t id, const size_t trId)
{
    size_t n = 0;

    for (size_t i = 0; i < this->transitions.size(); i++)
    {
        const State_Id_t trStateId = this->transitions[i].stA;
        if (trStateId == id)
        {
            if (trId == n)
            {
                return (&this->transitions[i]);
            }
            else
            {
                n++;
            }
        }
    }

    return (NULL);
}

size_t Reader_t::getDeclCount(const State_Id_t stateId, const Declaration_t type)
{
    size_t nDecl = 0;
    for (size_t i = 0; i < this->stateDeclarations.size(); i++)
    {
        if ((stateId == this->stateDeclarations[i].stateId) && (type == this->stateDeclarations[i].type))
        {
            nDecl++;
        }
    }
    return (nDecl);
}

State_Declaration_t* Reader_t::getDeclFromStateId(const State_Id_t stateId, const Declaration_t type, const size_t id)
{
    State_Declaration_t* decl = NULL;

    size_t nDecl = 0;
    for (size_t i = 0; i < this->stateDeclarations.size(); i++)
    {
        if ((stateId == this->stateDeclarations[i].stateId) && (type == this->stateDeclarations[i].type))
        {
            if (id == nDecl)
            {
                decl = &this->stateDeclarations[i];
                break; // for
            }
            nDecl++;
        }
    }

    return (decl);
}

// private function to collect all states in the document
void Reader_t::collectStates(void)
{
    // rewind
    this->in.clear();
    this->in.seekg(0);

    std::vector<State_Id_t> parentNesting;
    State_Id_t parentState = 0;
    bool isParentActive = false;

    std::string str;
    bool isuml = false;
    while (std::getline(this->in, str))
    {
        if (!isuml && (0 == str.compare("@startuml")))
        {
            // start parsing
            isuml = true;
        }
        else if (isuml && (0 == str.compare("@enduml")))
        {
            // end parsing
            isuml = false;
        }
        else if (isuml)
        {
            // parse line
            const std::vector<std::string> tokens = this->tokenize(str);
            if (5 <= tokens.size())
            {
                // possible format
                // S1 -> S2 : EV
                if ((0 == tokens[1].compare("->")) && (0 == tokens[3].compare(":")))
                {
                    // add index 0 and 2 as states
                    State_t stA;
                    stA.name = tokens[0];
                    stA.parent = parentState;
                    State_Id_t addrA = this->addState(stA);

                    State_t stB;
                    stB.name = (0 == tokens[2].compare("[*]")) ? "final" : tokens[2];
                    stB.parent = parentState;
                    State_Id_t addrB = this->addState(stB);

                    // add event
                    Event_t event;
                    event.name = tokens[4];
                    this->addEvent(event);

                    // add transition
                    Transition_t tr;
                    tr.stA = addrA;
                    tr.stB = addrB;
                    tr.event = event;
                    this->addTransition(tr);
                }
                // S : en/ex : body
                else if ((0 == tokens[1].compare(":")) && (0 == tokens[3].compare("/")))
                {
                    // fetch the state to attach the body.
                    for (size_t i = 0; i < this->states.size(); i++)
                    {
                        State_t* state = &this->states[i];
                        if ((0 == state->name.compare(tokens[0])) && (parentState == state->parent))
                        {
                            // assuming no duplicate states within a state, this
                            // should be fine.
                            State_Declaration_t decl;

                            // decode keyword
                            bool isValid = true;
                            if (0 == tokens[2].compare("entry"))
                            {
                                decl.type = Declaration_Entry;
                            }
                            else if (0 == tokens[2].compare("exit"))
                            {
                                decl.type = Declaration_Exit;
                            }
                            else if (0 == tokens[2].compare("oncycle"))
                            {
                                decl.type = Declaration_OnCycle;
                            }
                            else
                            {
                                // invalid keyword
                                isValid = false;
                            }

                            if (isValid)
                            {
                                // return to raw string and fetch all until end
                                size_t trim = str.find_first_of("/");
                                decl.stateId = state->id;
                                std::string tmpStr = str.substr(trim+1);
                                // trim beginning/end
                                trim = tmpStr.find_first_not_of(" ");
                                decl.declaration = tmpStr.substr(trim);
                                this->addDeclaration(decl);
                            }

                            break; // for
                        }
                    }
                }
            }
            if (3 == tokens.size())
            {
                // possible formats:
                // S1 -> S2
                if (0 == tokens[1].compare("->"))
                {
                    // add index 0 and 2 as states
                    State_t stA;
                    stA.name = (0 == tokens[0].compare("[*]")) ? "initial" : tokens[0];
                    stA.parent = parentState;
                    State_Id_t addrA = this->addState(stA);

                    State_t stB;
                    stB.name = (0 == tokens[2].compare("[*]")) ? "final" : tokens[2];
                    stB.parent = parentState;
                    State_Id_t addrB = this->addState(stB);

                    // add null transition
                    Event_t ev;
                    ev.name = "null";
                    Transition_t tr;
                    tr.event = ev;
                    tr.stA = addrA;
                    tr.stB = addrB;
                    this->addTransition(tr);
                }
                // state X {
                else if ((0 == tokens[0].compare("state")) && (0 == tokens[2].compare("{")))
                {
                    // add index 1 as state
                    State_t stA;
                    stA.name = tokens[1];
                    stA.parent = parentState;
                    State_Id_t addrA = this->addState(stA);

                    // push back nesting
                    if (isParentActive)
                    {
                        parentNesting.push_back(parentState);
                    }
                    parentState = addrA;
                }
            }
            else if (1 == tokens.size())
            {
                // possible end of x
                if (0 == tokens[0].compare("}"))
                {
                    // pop back parent nesting
                    if (0 < parentNesting.size())
                    {
                        parentState = parentNesting[parentNesting.size()-1];
                        parentNesting.pop_back();
                    }
                }
            }
        }
    }
}

State_Id_t Reader_t::addState(State_t newState)
{
    static State_Id_t id = 0;
    State_Id_t newId = 0;

    if (0 == this->states.size())
    {
        // just add the first
        id++;
        newState.id = id;
        this->states.push_back(newState);
        newId = newState.id;
    }
    else
    {
        // look for duplicate
        bool isFound = false;
        for (size_t i = 0; i < this->states.size(); i++)
        {
            if ((0 == this->states[i].name.compare(newState.name)) &&
                (newState.parent == this->states[i].parent))
            {
                isFound = true;
                newId = this->states[i].id;
                break; // for
            }
        }

        if (!isFound)
        {
            // new state
            id++;
            newState.id = id;
            this->states.push_back(newState);
            newId = newState.id;
        }
    }
    return (newId);
}

void Reader_t::addEvent(const Event_t newEvent)
{
    if (0 == this->events.size())
    {
        // just add the first
        this->events.push_back(newEvent);
    }
    else
    {
        // look for duplicate
        bool isFound = false;
        for (size_t i = 0; i < this->events.size(); i++)
        {
            if (0 == this->events[i].name.compare(newEvent.name))
            {
                isFound = true;
                break; // for
            }
        }

        if (!isFound)
        {
            // new event
            this->events.push_back(newEvent);
        }
    }
}

void Reader_t::addTransition(const Transition_t newTransition)
{
    this->transitions.push_back(newTransition);
}

void Reader_t::addDeclaration(const State_Declaration_t newDecl)
{
    this->stateDeclarations.push_back(newDecl);
}

void Reader_t::addFunction(const Function_t newFunc)
{
    this->functions.push_back(newFunc);
}

void Reader_t::addVariable(const Variable_t newVar)
{
    this->variables.push_back(newVar);
}

void Reader_t::addImport(const Import_t newImp)
{
    this->imports.push_back(newImp);
}

std::vector<std::string> Reader_t::tokenize(std::string str)
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

