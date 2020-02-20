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

void writePrototype_traceEntry(Writer_t* writer, const std::string modelName);
void writePrototype_traceExit(Writer_t* writer, const std::string modelName);

void writeImplementation_init(Writer_t* writer, Reader_t* reader, const State_t* firstState, const std::string modelName);
void writeImplementation_raiseEvent(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName);
void writeImplementation_clearInEvents(Writer_t* writer, Reader_t* reader, const size_t nEvents, const std::string modelName);
void writeImplementation_topRunCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_runCycle(Writer_t* writer, Reader_t* reader, const size_t nStates, const bool isParentFirstExec, const std::string modelName);
void writeImplementation_reactions(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_entryAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_exitAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);
void writeImplementation_oncycleAction(Writer_t* writer, Reader_t* reader, const size_t nStates, const std::string modelName);



