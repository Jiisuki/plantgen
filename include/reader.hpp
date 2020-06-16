/** @file
 *  @brief Class for reading a plantuml file.
 */

#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <fstream>

typedef size_t State_Id_t;

typedef struct
{
    State_Id_t id;
    std::string name;
    State_Id_t parent;
    bool isChoice;
} State_t;

typedef enum
{
    Declaration_Entry,
    Declaration_Exit,
    Declaration_OnCycle,
    Declaration_Comment,
} Declaration_t;

typedef struct
{
    State_Id_t stateId;
    Declaration_t type;
    std::string declaration;
} State_Declaration_t;

typedef enum
{
    Event_Direction_Incoming,
    Event_Direction_Outgoing,
    Event_Direction_Internal,
} Event_Direction_t;

typedef struct
{
    std::string name;
    bool requireParameter;
    std::string parameterType;
    bool isTimeEvent;
    Event_Direction_t direction;
    size_t expireTime_ms;
    bool isPeriodic;
} Event_t;

typedef struct
{
    State_Id_t stA;
    State_Id_t stB;
    Event_t event;
    bool hasGuard;
    std::string guard;
} Transition_t;

typedef struct
{
    bool isPrivate;
    std::string name;
    std::string type;
    bool specificInitialValue;
    std::string initialValue;
} Variable_t;

typedef struct
{
    bool isGlobal;
    std::string name;
} Import_t;

class Reader_t
{
    private:
        bool verbose;
        std::ifstream in;
        std::string modelName;
        std::vector<State_t> states;
        std::vector<Event_t> events;
        std::vector<Transition_t> transitions;
        std::vector<State_Declaration_t> stateDeclarations;
        std::vector<Variable_t> variables;
        std::vector<Import_t> imports;
        std::vector<std::string> uml;
        void collectStates(void);
        void collectEvents(void);
        std::vector<std::string> tokenize(std::string str);
        State_Id_t addState(State_t newState);
        Event_t addEvent(const Event_t newEvent);
        void addTransition(const Transition_t newTransition);
        void addDeclaration(const State_Declaration_t newDecl);
        void addVariable(const Variable_t newVar);
        void addImport(const Import_t newImp);
        void addUmlLine(const std::string line);

    public:
        Reader_t(std::string filename, const bool verbose);
        ~Reader_t();

        std::string getModelName(void);

        size_t getUmlLineCount(void);
        std::string getUmlLine(const size_t i);

        size_t getVariableCount(void);
        Variable_t* getVariable(const size_t id);

        size_t getPrivateVariableCount(void);
        Variable_t* getPrivateVariable(const size_t id);

        size_t getPublicVariableCount(void);
        Variable_t* getPublicVariable(const size_t id);

        size_t getImportCount(void);
        Import_t* getImport(const size_t id);

        size_t getStateCount(void);
        State_t* getState(const size_t id);
        State_t* getStateById(const State_Id_t id);

        size_t getInEventCount(void);
        Event_t* getInEvent(const size_t id);

        size_t getInternalEventCount(void);
        Event_t* getInternalEvent(const size_t id);

        size_t getTimeEventCount(void);
        Event_t* getTimeEvent(const size_t id);

        size_t getOutEventCount(void);
        Event_t* getOutEvent(const size_t id);

        size_t getTransitionCountFromStateId(const State_Id_t id);
        Transition_t* getTransitionFrom(const State_Id_t id, const size_t trId);

        size_t getDeclCount(const State_Id_t stateId, const Declaration_t type);
        State_Declaration_t* getDeclFromStateId(const State_Id_t stateId, const Declaration_t type, const size_t id);
};

