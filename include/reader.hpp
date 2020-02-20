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
} State_t;

typedef enum
{
    Declaration_Entry,
    Declaration_Exit,
    Declaration_OnCycle,
} Declaration_t;

typedef struct
{
    State_Id_t stateId;
    Declaration_t type;
    std::string declaration;
} State_Declaration_t;

typedef struct
{
    std::string name;
} Event_t;

typedef struct
{
    State_Id_t stA;
    State_Id_t stB;
    Event_t event;
} Transition_t;

typedef struct
{
    bool isPrivate;
    std::string type;
    std::string prototype;
} Function_t;

typedef struct
{
    bool isPrivate;
    std::string name;
    std::string type;
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
        std::ifstream in;
        std::string modelName;
        std::vector<State_t> states;
        std::vector<Event_t> events;
        std::vector<Transition_t> transitions;
        std::vector<State_Declaration_t> stateDeclarations;
        std::vector<Function_t> functions;
        std::vector<Variable_t> variables;
        std::vector<Import_t> imports;
        void collectStates(void);
        void collectEvents(void);
        std::vector<std::string> tokenize(std::string str);
        State_Id_t addState(State_t newState);
        void addEvent(const Event_t newEvent);
        void addTransition(const Transition_t newTransition);
        void addDeclaration(const State_Declaration_t newDecl);
        void addFunction(const Function_t newFunc);
        void addVariable(const Variable_t newVar);
        void addImport(const Import_t newImp);

    public:
        Reader_t(std::string filename);
        ~Reader_t();

        std::string getModelName(void);

        size_t getFunctionCount(void);
        Function_t* getFunction(const size_t id);

        size_t getVariableCount(void);
        Variable_t* getVariable(const size_t id);

        size_t getImportCount(void);
        Import_t* getImport(const size_t id);

        size_t getStateCount(void);
        State_t* getState(const size_t id);
        State_t* getStateById(const State_Id_t id);

        size_t getEventCount(void);
        Event_t* getEvent(const size_t id);

        size_t getTransitionCountFromStateId(const State_Id_t id);
        Transition_t* getTransitionFrom(const State_Id_t id, const size_t trId);

        size_t getDeclCount(const State_Id_t stateId, const Declaration_t type);
        State_Declaration_t* getDeclFromStateId(const State_Id_t stateId, const Declaration_t type, const size_t id);
};

