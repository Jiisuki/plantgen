/** @file
 *  @brief Handles writing of the generated code.
 */

#pragma once

#include <fstream>
#include <reader.hpp>
#include <style.hpp>

typedef struct
{
    bool verbose;
    bool doTracing;
    bool useSimpleNames;
    bool parentFirstExecution;
} Writer_Config_t;

class Writer_t
{
private:
    Writer_Config_t config;
    std::ofstream out_c;
    std::ofstream out_h;
    Reader_t* reader;
    Style_t* styler;
    void proto_runCycle(void);
    void proto_entryAction(void);
    void proto_exitAction(void);
    void proto_raiseInEvent(void);
    void proto_raiseOutEvent(void);
    void proto_raiseInternalEvent(void);
    void proto_timeTick(void);
    void proto_checkOutEvent(void);
    void proto_clearEvents(void);
    void proto_getVariable(void);
    void proto_traceEntry(void);
    void proto_traceExit(void);
    void decl_stateList(void);
    void decl_eventList(void);
    void decl_variableList(void);
    void decl_stateMachine(void);
    void impl_init(std::vector<State_t*> firstState);
    void impl_raiseInEvent(void);
    void impl_raiseOutEvent(void);
    void impl_raiseInternalEvent(void);
    void impl_checkOutEvent(void);
    void impl_getVariable(void);
    void impl_timeTick(void);
    void impl_clearEvents(void);
    void impl_topRunCycle(void);
    void impl_runCycle(void);
    void impl_entryAction(void);
    void impl_exitAction(void);
    std::vector<std::string> tokenize(std::string str);
    void parseDeclaration(const std::string declaration);
    std::string parseGuard(const std::string guardStrRaw);
    void parseChoicePath(State_t* initialChoice, size_t indentLevel);
    std::vector<State_t*> getChildStates(State_t* currentState);
    bool parseChildExits(State_t* currentState, size_t indentLevel, const State_Id_t topState, const bool didPreviousWrite);
    bool hasEntryStatement(const State_Id_t stateId);
    bool hasExitStatement(const State_Id_t stateId);
    std::string getTraceCall_entry(const State_t* state);
    std::string getTraceCall_exit(const State_t* state);
    std::vector<State_t*> findInitState(void);
    std::vector<State_t*> findEntryState(State_t* in);
    std::vector<State_t*> findFinalState(State_t* in);
    std::string getIndent(const size_t level);
    std::string getIfElseIf(const size_t i);
    void errorReport(std::string str, unsigned int line);

public:
    Writer_t();
    ~Writer_t();
    void generateCode(const std::string filename, const std::string outdir);
    void enableVerbose(void);
    void enableSimpleNames(void);
    void enableTracing(void);
    void enableParentFirstExecution(void);
};



