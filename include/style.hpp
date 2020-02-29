/*
 * style.h
 *
 *  Created on: 29 feb. 2020
 *      Author: Jiisuki
 */

#pragma once

#include <string>
#include "reader.hpp"

class Style_t
{
private:
    bool useSimpleNames;
    Reader_t* reader;
    std::string appendModelName(const std::string str);
    std::string getStateBaseDecl(const State_t* state);
public:
    Style_t(Reader_t* reader);
    ~Style_t();
    void enableSimpleNames(void);
    void disableSimpleNames(void);
    std::string getTopRunCycle(void);
    std::string getStateRunCycle(const State_t* state);
    std::string getStateEntry(const State_t* state);
    std::string getStateExit(const State_t* state);
    std::string getStateName(const State_t* state);
    std::string getStateType(void);
    std::string getEventRaise(const Event_t* event);
    std::string getEventRaise(const std::string eventName);
    std::string getHandleType(void);
    std::string getTimeTick(void);
    std::string getEventIsRaised(const Event_t* event);
    std::string getVariable(const Variable_t* var);
    std::string getTraceEntry(void);
    std::string getTraceExit(void);
};
