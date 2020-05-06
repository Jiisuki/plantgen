/*
 * style.cpp
 *
 *  Created on: 29 feb. 2020
 *      Author: Jiisuki
 */

#include "style.hpp"
#include <string>
#include <reader.hpp>

Style_t::Style_t(Reader_t* reader)
{
    this->reader = reader;
    this->useSimpleNames = false;
}

Style_t::~Style_t()
{
    /* It is not up to us to cleanup. */
}

void Style_t::enableSimpleNames(void)
{
    this->useSimpleNames = true;
}

void Style_t::disableSimpleNames(void)
{
    this->useSimpleNames = false;
}

std::string Style_t::appendModelName(const std::string str)
{
    return (this->reader->getModelName() + "_" + str);
}

std::string Style_t::getStateBaseDecl(const State_t* state)
{
    std::string decl_base = "";

    if (true != this->useSimpleNames)
    {
        State_t* parent = reader->getStateById(state->parent);
        if (NULL != parent)
        {
            decl_base = getStateBaseDecl(parent) + "_";
        }
    }

    /* Add first letter upper-case. */
    decl_base += std::toupper(state->name[0]);
    /* Add rest as is. */
    decl_base += state->name.substr(1);
    //decl_base += state->name;

    return (decl_base);
}

std::string Style_t::getTopRunCycle(void)
{
    return (this->appendModelName("runCycle"));
}

std::string Style_t::getStateRunCycle(const State_t* state)
{
    return (this->appendModelName(this->getStateBaseDecl(state) + "_react"));
}

std::string Style_t::getStateEntry(const State_t* state)
{
    return (this->appendModelName(this->getStateBaseDecl(state) + "_entryAction"));
}

std::string Style_t::getStateExit(const State_t* state)
{
    return (this->appendModelName(this->getStateBaseDecl(state) + "_exitAction"));
}

std::string Style_t::getStateName(const State_t* state)
{
    return (this->appendModelName("State_" + this->getStateBaseDecl(state)));
}

std::string Style_t::getStateType(void)
{
    return (this->appendModelName("State_t"));
}

std::string Style_t::getEventRaise(const Event_t* event)
{
    return (this->appendModelName("raise_" + event->name));
}

std::string Style_t::getEventRaise(const std::string eventName)
{
    return (this->appendModelName("raise_" + eventName));
}

std::string Style_t::getHandleType(void)
{
    return (this->appendModelName("Handle_t"));
}

std::string Style_t::getTimeTick(void)
{
    return (this->appendModelName("_timeTick"));
}

std::string Style_t::getEventIsRaised(const Event_t* event)
{
    return (this->appendModelName("is_" + event->name + "_raised"));
}

std::string Style_t::getVariable(const Variable_t* var)
{
    return (this->appendModelName("get_" + var->name));
}

std::string Style_t::getTraceEntry(void)
{
    return (this->appendModelName("traceEntry"));
}

std::string Style_t::getTraceExit(void)
{
    return (this->appendModelName("traceExit"));
}
