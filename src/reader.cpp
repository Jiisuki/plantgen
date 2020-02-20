#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <reader.hpp>

Reader_t::Reader_t(std::string filename, const bool verbose)
{
    this->verbose = verbose;
    this->indentLevel = 0;
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
size_t Reader_t::getUmlLineCount(void)
{
    return (this->uml.size());
}

// public
std::string Reader_t::getUmlLine(const size_t i)
{
    return (this->uml[i]);
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
    return (this->imports.size());
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
size_t Reader_t::getInEventCount(void)
{
    return (this->inEvents.size());
}

// public
Event_t* Reader_t::getInEvent(size_t id)
{
    Event_t* event = NULL;
    if (id < this->inEvents.size())
    {
        event = &this->inEvents[id];
    }
    return (event);
}

// public
size_t Reader_t::getOutEventCount(void)
{
    return (this->outEvents.size());
}

// public
Event_t* Reader_t::getOutEvent(size_t id)
{
    Event_t* event = NULL;
    if (id < this->outEvents.size())
    {
        event = &this->outEvents[id];
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

static bool isTrArrow(const std::string token)
{
    bool isArrow = false;
    char first = token.front();
    char last = token.back();
    if (('-' == first) && ('>' == last))
    {
        isArrow = true;
    }
    return (isArrow);
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
    bool isheader = false;
    bool isfooter = false;

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
            this->addUmlLine(str);

            if (0 == str.compare("header"))
            {
                // TODO: start header parsing
                isheader = true;
            }
            else if (0 == str.compare("footer"))
            {
                // TODO: start footer parsing
                isfooter = true;
            }
            else if (0 == str.compare("endheader"))
            {
                // TODO: end header parsing
                isheader = false;
            }
            else if (0 == str.compare("endfooter"))
            {
                // TODO: end footer parsing
                isfooter = false;
            }
            else if (isheader || isfooter)
            {
                // parse header/footer
                const std::vector<std::string> tokens = this->tokenize(str);

                if (0 < tokens.size())
                {
                    if ((0 == tokens[0].compare("model")) && (2 == tokens.size()))
                    {
                        this->modelName = tokens[1];

                        if (this->verbose)
                        {
                            std::cout << this->getIndent() << "Model name detected: " << this->modelName << std::endl;
                        }
                    }
                    else if ((0 == tokens[0].compare("import")) && (3 <= tokens.size()))
                    {
                        Import_t newImport;
                        if ((0 == tokens[1].compare("global")) && (4 == tokens.size()))
                        {
                            newImport.isGlobal = true;
                            newImport.name = tokens[3];
                        }
                        else
                        {
                            newImport.isGlobal = false;
                            newImport.name = tokens[2];
                        }
                        this->addImport(newImport);
                    }
                    else if (((0 == tokens[0].compare("private")) || (0 == tokens[0].compare("public"))) && (5 <= tokens.size()))
                    {
                        Variable_t newVariable;
                        newVariable.isPrivate = (0 == tokens[0].compare("private")) ? true : false;
                        newVariable.name = tokens[2];
                        newVariable.type = tokens[4];
                        if (7 == tokens.size())
                        {
                            newVariable.specificInitialValue = true;
                            newVariable.initialValue = tokens[6];
                        }
                        else
                        {
                            newVariable.specificInitialValue = false;
                            newVariable.initialValue = "";
                        }
                        this->addVariable(newVariable);
                    }
                    else if (((0 == tokens[0].compare("in")) || (0 == tokens[0].compare("out"))) && (0 == tokens[1].compare("event")) && (3 <= tokens.size()))
                    {
                        Event_t newEvent;
                        newEvent.name = tokens[2];
                        if (5 == tokens.size())
                        {
                            newEvent.requireParameter = true;
                            newEvent.parameterType = tokens[4];
                        }
                        else
                        {
                            newEvent.requireParameter = false;
                            newEvent.parameterType = "";
                        }
                        if (0 == tokens[0].compare("in"))
                        {
                            this->addInEvent(newEvent);
                        }
                        else
                        {
                            this->addOutEvent(newEvent);
                        }
                    }
                }
            }
            else
            {
                // parse line
                const std::vector<std::string> tokens = this->tokenize(str);
                const size_t numTokens = tokens.size();
                if (0 < numTokens)
                {
                    // =========================================================
                    // STATE DEFINITION # state X Y
                    // =========================================================
                    if ((0 == tokens[0].compare("state")) && (1 < numTokens))
                    {
                        // define a state
                        State_t state;
                        state.name = tokens[1];
                        state.parent = parentState;
                        state.isChoice = false;

                        bool setNewParent = false;
                        if (2 < numTokens)
                        {
                            // check for special
                            if (0 == tokens[2].compare("<<choice>>"))
                            {
                                state.isChoice = true;
                            }
                            else if (0 == tokens[2].compare("{"))
                            {
                                // parent nesting
                                setNewParent = true;
                            }
                        }

                        const size_t id = this->addState(state);
                        if (setNewParent)
                        {
                            if (0 != parentState)
                            {
                                parentNesting.push_back(parentState);
                            }
                            parentState = id;
                        }
                    }

                    // =========================================================
                    // STATE TRANSITION # S1 -> S2 : X Y
                    // If X is a guard [X] then [Y] is considered a part of this
                    // If X is an event X then [Y] is considered its guard
                    // =========================================================
                    else if ((2 < numTokens) && (isTrArrow(tokens[1])))
                    {
                        State_t A, B;
                        A.name = (0 == tokens[0].compare("[*]")) ? "initial" : tokens[0];
                        A.isChoice = false; // relies on already defined state.
                        A.parent = parentState;

                        B.name = (0 == tokens[2].compare("[*]")) ? "final" : tokens[2];
                        B.isChoice = false; // see above.
                        B.parent = parentState;

                        const size_t idA = this->addState(A);
                        const size_t idB = this->addState(B);

                        Event_t ev;
                        ev.name = "null";
                        ev.parameterType = "";
                        ev.requireParameter = false;

                        Transition_t tr;
                        tr.stA = idA;
                        tr.stB = idB;
                        tr.hasGuard = false;

                        if ((4 < numTokens) && (0 == tokens[3].compare(":")))
                        {
                            if ('[' == tokens[4].front())
                            {
                                // guard only transition.
                                tr.hasGuard = true;
                                std::string guardStr = tokens[4];
                                for (size_t i = 5; i < tokens.size(); i++)
                                {
                                    guardStr += " " + tokens[i];
                                }
                                tr.guard = guardStr.substr(1, guardStr.length()-2);
                            }
                            else
                            {
                                // transition is on event.
                                ev.name = tokens[4];

                                if ((5 < numTokens) && ('[' == tokens[5].front()))
                                {
                                    // guard on transition.
                                    tr.hasGuard = true;
                                    std::string guardStr = tokens[5];
                                    for (size_t i = 6; i < tokens.size(); i++)
                                    {
                                        guardStr += " " + tokens[i];
                                    }
                                    tr.guard = guardStr.substr(1, guardStr.length()-2);
                                }
                            }
                        }

                        // add event to transition and add transition.
                        tr.event = ev;
                        this->addTransition(tr);
                    }

                    // =========================================================
                    // STATE ACTION # S : T / X
                    // T is entry/exit/oncycle and X is the action for state S.
                    // =========================================================
                    else if ((2 < numTokens) && (0 == tokens[1].compare(":")))
                    {
                        // action
                        State_Id_t id = 0;
                        for (size_t i = 0; i < this->states.size(); i++)
                        {
                            if (0 == this->states[i].name.compare(tokens[0]))
                            {
                                id = this->states[i].id;
                                break;
                            }
                        }

                        if (0 != id)
                        {
                            if ((3 < numTokens) && (0 == tokens[3].compare("/")))
                            {
                                State_Declaration_t d;
                                d.stateId = id;

                                bool hasType = true;
                                if (0 == tokens[2].compare("entry"))
                                {
                                    d.type = Declaration_Entry;
                                }
                                else if (0 == tokens[2].compare("exit"))
                                {
                                    d.type = Declaration_Exit;
                                }
                                else if (0 == tokens[2].compare("oncycle"))
                                {
                                    d.type = Declaration_OnCycle;
                                }
                                else
                                {
                                    hasType = false;
                                }
                                if (hasType)
                                {
                                    std::string declStr = tokens[4];
                                    for (size_t i = 5; i < tokens.size(); i++)
                                    {
                                        declStr += " " + tokens[i];
                                    }
                                    d.declaration = declStr;
                                    this->addDeclaration(d);
                                }
                            }
                            else
                            {
                                // comment
                                State_Declaration_t d;
                                d.stateId = id;
                                d.type = Declaration_Comment;

                                std::string declStr = tokens[2];
                                for (size_t i = 3; i < tokens.size(); i++)
                                {
                                    declStr += " " + tokens[i];
                                }
                                d.declaration = declStr;
                                this->addDeclaration(d);
                            }
                        }
                    }
                }
#if 0
                if (5 <= tokens.size())
                {
                    // possible format
                    // S1 -> S2 : EV
                    if (isTrArrow(tokens[1]) && (0 == tokens[3].compare(":")))
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

                        if ('[' == tokens[4].front())
                        {
                            // guard only transition
                            Event_t event;
                            event.name = "null";
                            event.requireParameter = false;
                            event.parameterType = "";

                            Transition_t tr;
                            tr.stA = addrA;
                            tr.stB = addrB;
                            tr.event = event;

                            // check for guard
                            if (4 < tokens.size())
                            {
                                std::string guardStr = tokens[4];
                                for (size_t i = 5; i < tokens.size(); i++)
                                {
                                    guardStr += " " + tokens[i];
                                }
                                tr.guard = guardStr.substr(1, guardStr.length()-2);
                                tr.hasGuard = true;
                            }
                            else
                            {
                                tr.guard = "";
                                tr.hasGuard = false;
                            }

                            this->addTransition(tr);
                        }
                        else
                        {
                            // add event
                            Event_t event;
                            event.name = tokens[4];
                            event.requireParameter = false;
                            event.parameterType = "";
                            this->addInEvent(event);

                            // add transition
                            Transition_t tr;
                            tr.stA = addrA;
                            tr.stB = addrB;
                            tr.event = event;

                            // check for guard
                            if (5 < tokens.size())
                            {
                                std::string guardStr = tokens[5];
                                for (size_t i = 6; i < tokens.size(); i++)
                                {
                                    guardStr += " " + tokens[i];
                                }
                                tr.guard = guardStr.substr(1, guardStr.length()-2);
                                tr.hasGuard = true;
                            }
                            else
                            {
                                tr.guard = "";
                                tr.hasGuard = false;
                            }

                            if (this->verbose)
                            {
                                if (tr.hasGuard)
                                {
                                    std::cout << "with guard." << std::endl;
                                }
                                else
                                {
                                    std::cout << std::endl;
                                }
                            }

                            this->addTransition(tr);
                        }
                    }
                    // S : en/ex : body
                    else if ((0 == tokens[1].compare(":")) && (0 == tokens[3].compare("/")))
                    {
                        // fetch the state to attach the body.
                        for (size_t i = 0; i < this->states.size(); i++)
                        {
                            State_t* state = &this->states[i];
                            if (0 == state->name.compare(tokens[0]))
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
                if (3 <= tokens.size())
                {
                    // possible formats:
                    // S1 -> S2
                    if (isTrArrow(tokens[1]))
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
                        ev.requireParameter = false;
                        ev.parameterType = "";

                        Transition_t tr;
                        tr.event = ev;
                        tr.stA = addrA;
                        tr.stB = addrB;
                        tr.guard = "";
                        tr.hasGuard = false;

                        this->addTransition(tr);
                    }
                    // state X
                    else if (0 == tokens[0].compare("state"))
                    {
                        // state X <<choice>>
                        if (0 == tokens[2].compare("<<choice>>"))
                        {
                            State_t stA;
                            stA.name = tokens[1];
                            stA.parent = parentState;
                            stA.isChoice = true;
                            this->addState(stA);
                        }
                        else if (0 == tokens[2].compare("{"))
                        {
                            // add index 1 as state
                            State_t stA;
                            stA.name = tokens[1];
                            stA.parent = parentState;
                            stA.isChoice = false;
                            State_Id_t addrA = this->addState(stA);

                            if (this->verbose)
                            {
                                std::cout << this->getIndent() << tokens[1] << " {" << std::endl;
                            }

                            // push back nesting
                            if (isParentActive)
                            {
                                parentNesting.push_back(parentState);
                            }
                            parentState = addrA;
                            isParentActive = true;
                            this->indentLevel++;
                        }
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
                        else
                        {
                            parentState = 0;
                            isParentActive = false;
                        }

                        if (0 < this->indentLevel)
                        {
                            this->indentLevel--;
                        }

                        if (this->verbose)
                        {
                            std::cout << this->getIndent() << "}" << std::endl;
#if 0
                            if ((0 != parentState) && (isParentActive))
                            {
                                State_t* st = this->getStateById(parentState);
                                std::cout << this->getIndent() << "New parent: " << st->name << std::endl;
                            }
#endif
                        }
                    }
                }
#endif
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
            if ((0 == newState.name.compare("initial")) || (0 == newState.name.compare("final")))
            {
                // check if this exact one exists already
                if ((0 == this->states[i].name.compare(newState.name)) && (newState.parent == this->states[i].parent))
                {
                    isFound = true;
                    newId = this->states[i].id;
                    break;
                }
            }
            else
            {
                // require unique names
                if (0 == this->states[i].name.compare(newState.name))
                {
                    isFound = true;
                    newId = this->states[i].id;
                    break; // for
                }
            }
        }

        if (!isFound)
        {
            // new state
            id++;
            newState.id = id;
            this->states.push_back(newState);
            newId = newState.id;

            if (this->verbose)
            {
                std::cout << this->getIndent() << "Added new state " << newState.name << " [id = " << newState.id << "]" << std::endl;
            }
        }
    }
    return (newId);
}

void Reader_t::addInEvent(const Event_t newEvent)
{
    if (0 == this->inEvents.size())
    {
        // just add the first
        this->inEvents.push_back(newEvent);
    }
    else
    {
        // look for duplicate
        std::string dupEvStr = "";
        bool isFound = false;
        for (size_t i = 0; i < this->inEvents.size(); i++)
        {
            if (0 == this->inEvents[i].name.compare(newEvent.name))
            {
                isFound = true;
                dupEvStr = this->inEvents[i].name;
                break; // for
            }
        }

        if (!isFound)
        {
            // new event
            this->inEvents.push_back(newEvent);

            if (this->verbose)
            {
                std::cout << this->getIndent() << "Added new incoming event " << newEvent.name << std::endl;
            }
        }
    }
}

void Reader_t::addOutEvent(const Event_t newEvent)
{
    if (0 == this->outEvents.size())
    {
        // just add the first
        this->outEvents.push_back(newEvent);
    }
    else
    {
        // look for duplicate
        bool isFound = false;
        for (size_t i = 0; i < this->outEvents.size(); i++)
        {
            if (0 == this->outEvents[i].name.compare(newEvent.name))
            {
                isFound = true;
                break; // for
            }
        }

        if (!isFound)
        {
            // new event
            this->outEvents.push_back(newEvent);

            if (this->verbose)
            {
                std::cout << this->getIndent() << "Added new outgoing event " << newEvent.name << std::endl;
            }
        }
    }
}

void Reader_t::addTransition(const Transition_t newTransition)
{
    this->transitions.push_back(newTransition);

    if (this->verbose)
    {
        State_t* A = this->getStateById(newTransition.stA);
        State_t* B = this->getStateById(newTransition.stB);
        std::string stateA = (NULL == A) ? "NULL" : A->name;
        std::string stateB = (NULL == B) ? "NULL" : B->name;
        std::string guardDesc = (newTransition.hasGuard) ? (" with guard [" + newTransition.guard + "]") : "";
        std::cout << this->getIndent()
                  << "Added transition "
                  << stateA
                  << " --> "
                  << stateB
                  << " on event "
                  << newTransition.event.name
                  << guardDesc
                  << std::endl;
    }
}

void Reader_t::addDeclaration(const State_Declaration_t newDecl)
{
    this->stateDeclarations.push_back(newDecl);

    if (this->verbose)
    {
        State_t* st = this->getStateById(newDecl.stateId);
        std::string name = (NULL == st) ? "NULL" : st->name;
        std::string type = "NULL";
        if (Declaration_Entry == newDecl.type)
        {
            type = "Entry";
        }
        else if (Declaration_Exit == newDecl.type)
        {
            type = "Exit";
        }
        else if (Declaration_OnCycle == newDecl.type)
        {
            type = "OnCycle";
        }
        std::cout << this->getIndent() << "Wrote " << type << " declaration for state " << name << std::endl;
    }
}

void Reader_t::addVariable(const Variable_t newVar)
{
    this->variables.push_back(newVar);

    if (this->verbose)
    {
        std::cout << this->getIndent() << "Found variable " << newVar.type << " " << newVar.name;
        if (newVar.specificInitialValue)
        {
            std::cout << " = " << newVar.initialValue << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
    }
}

void Reader_t::addImport(const Import_t newImp)
{
    this->imports.push_back(newImp);

    if (this->verbose)
    {
        std::cout << this->getIndent() << "Found import ";
        if (newImp.isGlobal)
        {
            std::cout << "<" << newImp.name << ">" << std::endl;
        }
        else
        {
            std::cout << "\"" << newImp.name << "\"" << std::endl;
        }
    }
}

void Reader_t::addUmlLine(const std::string line)
{
    this->uml.push_back(line);
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

std::string Reader_t::getIndent(void)
{
    std::string str = "";
    for (size_t i = 0; i < this->indentLevel; i++)
    {
        str += "  ";
    }
    return (str);
}

