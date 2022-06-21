/** @file
 *  @brief Handles writing of the generated code.
 */

#pragma once

#include <fstream>
#include "reader.hpp"
#include "style.hpp"

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
    std::string filename;
    std::string outdir;
    std::ofstream out_c;
    std::ofstream out_h;
    Reader_t reader;
    Style_t styler;
    void proto_runCycle();
    void proto_entryAction();
    void proto_exitAction();
    void proto_raiseInEvent();
    void proto_raiseOutEvent();
    void proto_raiseInternalEvent();
    void proto_timeTick();
    void proto_checkOutEvent();
    void proto_clearEvents();
    void proto_getVariable();
    void decl_stateList();
    void decl_eventList();
    void decl_variableList();
    void decl_tracingCallback();
    void decl_stateMachine();
    void impl_init(const std::vector<State_t*>& firstState);
    void impl_raiseInEvent();
    void impl_raiseOutEvent();
    void impl_raiseInternalEvent();
    void impl_checkOutEvent();
    void impl_getVariable();
    void impl_timeTick();
    void impl_clearEvents();
    void impl_checkAnyEventRaised();
    void impl_topRunCycle();
    void impl_traceCalls();
    void impl_runCycle();
    void impl_entryAction();
    void impl_exitAction();
    std::vector<std::string> tokenize(std::string str);
    void parseDeclaration(const std::string& declaration);
    std::string parseGuard(const std::string& guardStrRaw);
    void parseChoicePath(State_t* initialChoice, size_t indentLevel);
    std::vector<State_t*> getChildStates(State_t* currentState);
    bool parseChildExits(State_t* currentState, size_t indentLevel, State_Id_t topState, bool didPreviousWrite);
    bool hasEntryStatement(State_Id_t stateId);
    bool hasExitStatement(State_Id_t stateId);
    std::string getTraceCall_entry(const State_t* state);
    std::string getTraceCall_exit(const State_t* state);
    std::vector<State_t*> findInitState();
    std::vector<State_t*> findEntryState(State_t* in);
    std::vector<State_t*> findFinalState(State_t* in);
    std::string getIndent(size_t level);
    std::string getIfElseIf(size_t i);
    void errorReport(const std::string& str, unsigned int line);

public:
    Writer_t(const std::string& filename, const std::string& outdir, const Writer_Config_t& cfg);
    ~Writer_t();
    void generateCode();
};



