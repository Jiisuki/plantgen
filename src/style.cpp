/** @file
 *  @brief Implementation of the styling class.
 */

#include <string>
#include "../include/style.hpp"
#include "../include/reader.hpp"

Style_t::Style_t(Reader_t& reader) : reader(reader), useSimpleNames(false)
{}

Style_t::~Style_t()
{
    /* It is not up to us to cleanup. */
}

void Style_t::enableSimpleNames()
{
    this->useSimpleNames = true;
}

void Style_t::disableSimpleNames()
{
    this->useSimpleNames = false;
}

std::string Style_t::appendModelName(const std::string& str)
{
    return (reader.getModelName() + "_" + str);
}

std::string Style_t::getStateBaseDecl(const State_t* state)
{
    std::string decl_base = "";

    if (!useSimpleNames)
    {
        State_t* parent = reader.getStateById(state->parent);
        if (nullptr != parent)
        {
            decl_base = getStateBaseDecl(parent) + "_";
        }
    }

    /* Add first letter upper-case. */
    decl_base += static_cast<char>(std::toupper(state->name[0]));

    /* Add rest as is. */
    decl_base += state->name.substr(1);
    //decl_base += state->name;

    return (decl_base);
}

std::string Style_t::getTopRunCycle()
{
    return "run_cycle";
}

std::string Style_t::getStateRunCycle(const State_t* state)
{
    auto b = this->getStateBaseDecl(state);
    if (!b.empty())
    {
        b[0] = static_cast<char>(std::tolower(b[0]));
    }
    return "state_" + b + "_react";
}

std::string Style_t::getStateEntry(const State_t* state)
{
    auto b = this->getStateBaseDecl(state);
    if (!b.empty())
    {
        b[0] = static_cast<char>(std::tolower(b[0]));
    }
    return "state_" + b + "_entry_action";
}

std::string Style_t::getStateExit(const State_t* state)
{
    auto b = this->getStateBaseDecl(state);
    if (!b.empty())
    {
        b[0] = static_cast<char>(std::tolower(b[0]));
    }
    return "state_" + b + "_exit_action";
}

std::string Style_t::getStateName(const State_t* state)
{
    return (this->appendModelName("State") + "::" + this->getStateBaseDecl(state));
}

std::string Style_t::getStateNamePure(const State_t* state)
{
    return this->getStateBaseDecl(state);
}

std::string Style_t::getStateType()
{
    return (this->appendModelName("State"));
}

std::string Style_t::getEventRaise(const Event_t* event)
{
    return "raise_" + event->name;
}

std::string Style_t::getEventRaise(const std::string& eventName)
{
    return "raise_" + eventName;
    //return (this->appendModelName("raise_" + eventName));
}

std::string Style_t::getTimeTick()
{
    return "time_tick";
}

std::string Style_t::getEventIsRaised(const Event_t* event)
{
    return "is_" + event->name + "_raised";
    //return (this->appendModelName("is_" + event->name + "_raised"));
}

std::string Style_t::getEventValue(const Event_t* event)
{
    return "get_" + event->name + "_value";
    //return (this->appendModelName("is_" + event->name + "_raised"));
}

std::string Style_t::getVariable(const Variable_t* var)
{
    return "get_" + var->name;
    //return (this->appendModelName("get_" + var->name));
}

std::string Style_t::getTraceEntry()
{
    return "trace_state_enter";
}

std::string Style_t::getTraceExit()
{
    return "trace_state_exit";
}
