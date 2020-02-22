#pragma once

#include <fstream>
#include <reader.hpp>

typedef struct
{
    std::ofstream out_c;
    std::ofstream out_h;
} Writer_t;

void Writer_setTracing(const bool setting);

void writePrototype_runCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writePrototype_react(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writePrototype_entryAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writePrototype_exitAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writePrototype_raiseInEvent(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName);
void writePrototype_raiseOutEvent(Writer_t* writer, Reader_t* reader);
void writePrototype_timeTick(Writer_t* writer, Reader_t* reader);
void writePrototype_checkOutEvent(Writer_t* writer, Reader_t* reader);
void writePrototype_clearEvents(Writer_t* writer, Reader_t* reader);
void writePrototype_getVariable(Writer_t* writer, Reader_t* reader);

void writePrototype_traceEntry(Writer_t* writer, const std::string modelName);
void writePrototype_traceExit(Writer_t* writer, const std::string modelName);

void writeDeclaration_stateList(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeDeclaration_eventList(Writer_t* writer, Reader_t* reader);
void writeDeclaration_variableList(Writer_t* writer, Reader_t* reader, const size_t nVariables, const std::string modelName);
void writeDeclaration_stateMachine(Writer_t* writer, Reader_t* reader, const std::string modelName);

void writeImplementation_init(Writer_t* writer, Reader_t* reader, const std::vector<State_t*> firstState, const std::string modelName);
void writeImplementation_raiseInEvent(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName);
void writeImplementation_raiseOutEvent(Writer_t* writer, Reader_t* reader);
void writeImplementation_checkOutEvent(Writer_t* writer, Reader_t* reader);
void writeImplementation_getVariable(Writer_t* writer, Reader_t* reader);
void writeImplementation_timeTick(Writer_t* writer, Reader_t* reader);
void writeImplementation_clearEvents(Writer_t* writer, Reader_t* reader);
void writeImplementation_topRunCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_runCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const bool isParentFirstExec, const std::string modelName);
void writeImplementation_reactions(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_entryAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_exitAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);



