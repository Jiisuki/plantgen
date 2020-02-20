#pragma once

#include <string>
#include <iostream>

#include <reader.hpp>

typedef enum
{
    Error,
    Warning,
} Error_t;

void errorReport(const Error_t severity, std::string str, unsigned int line);

std::string getStateBaseDecl(Reader_t* reader, const State_t* state);
std::string getStateRunCycle(Reader_t* reader, const std::string modelName, const State_t* state);
std::string getStateReact(Reader_t* reader, const std::string modelName, const State_t* state);
std::string getStateEntry(Reader_t* reader, const std::string modelName, const State_t* state);
std::string getStateExit(Reader_t* reader, const std::string modelName, const State_t* state);
std::string getStateOnCycle(Reader_t* reader, const std::string modelName, const State_t* state);
std::string getStateName(Reader_t* reader, const std::string modelName, const State_t* state);
std::string getEventRaise(Reader_t* reader, const std::string modelName, const Event_t* event);
std::string getHandleName(const std::string modelName);

std::string getTraceCall_entry(const std::string modelName);
std::string getTraceCall_exit(const std::string modelName);

State_t* findEntryState(Reader_t* reader, const std::string modelName, State_t* in);
State_t* findFinalState(Reader_t* reader, const std::string modelName, State_t* in);
State_t* findInitState(Reader_t* reader, const size_t nStates, const std::string modelName);

std::string getIndent(const size_t level);
std::string getIfElseIf(const size_t i);

#define ERROR_REPORT(severity, str) errorReport((severity), (str), __LINE__)

