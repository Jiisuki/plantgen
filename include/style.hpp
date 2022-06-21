/** @file
 *  @brief Handles styling of the generated code.
 */

#pragma once

#include <string>
#include "reader.hpp"

class Style_t
{
private:
    bool useSimpleNames;
    Reader_t& reader;
    std::string appendModelName(const std::string& str);
    std::string getStateBaseDecl(const State_t* state);
public:
    explicit Style_t(Reader_t& reader);
    ~Style_t();
    void enableSimpleNames();
    void disableSimpleNames();
    std::string getTopRunCycle();
    std::string getStateRunCycle(const State_t* state);
    std::string getStateEntry(const State_t* state);
    std::string getStateExit(const State_t* state);
    std::string getStateName(const State_t* state);
    std::string getStateNamePure(const State_t* state);
    std::string getStateType();
    std::string getEventRaise(const Event_t* event);
    std::string getEventRaise(const std::string& eventName);
    std::string getTimeTick();
    std::string getEventIsRaised(const Event_t* event);
    std::string getEventValue(const Event_t* event);
    std::string getVariable(const Variable_t* var);
    std::string getTraceEntry();
    std::string getTraceExit();
};
