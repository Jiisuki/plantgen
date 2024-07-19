/** @file
 *  @brief Class for reading a plantuml file.
 */

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using StateId = size_t;

struct State
{
    StateId     id;
    std::string name;
    StateId     parent;
    bool        is_choice;

    State() : id(), name(), parent(), is_choice() {}
    ~State() = default;
};

enum class Declaration
{
    Entry,
    Exit,
    OnCycle,
    Comment,
};

struct StateDeclaration
{
    StateId     state_id;
    Declaration type;
    std::string declaration;

    StateDeclaration() : state_id(), type(), declaration() {}
    ~StateDeclaration() = default;
};

enum class EventDirection
{
    Incoming,
    Outgoing,
    Internal,
};

struct Event
{
    std::string    name;
    bool           require_parameter;
    std::string    parameter_type;
    bool           is_time_event;
    EventDirection direction;
    size_t         expire_time_ms;
    bool           is_periodic;

    Event() :
        name(), require_parameter(), parameter_type(), is_time_event(), direction(), expire_time_ms(), is_periodic()
    {
    }
    ~Event() = default;
};

struct Transition
{
    StateId     state_a;
    StateId     state_b;
    Event       event;
    bool        has_guard;
    std::string guard;

    Transition() : state_a(), state_b(), event(), has_guard(), guard() {}
    ~Transition() = default;
};

struct Variable
{
    bool        is_private;
    std::string name;
    std::string type;
    bool        specific_initial_value;
    std::string initial_value;

    Variable() : is_private(), name(), type(), specific_initial_value(), initial_value() {}
    ~Variable() = default;
};

struct Import
{
    bool        is_global;
    std::string name;

    Import() : is_global(), name() {}
    ~Import() = default;
};

class Reader
{
  private:
    bool                          verbose;
    std::ifstream                 in;
    std::string                   model_name;
    std::vector<State>            states;
    std::vector<Event>            events;
    std::vector<Transition>       transitions;
    std::vector<StateDeclaration> state_declarations;
    std::vector<Variable>         variables;
    std::vector<Import>           imports;
    std::vector<std::string>      uml;

    void                            collect_states();
    static std::vector<std::string> tokenize(const std::string& str);
    StateId                         add_state(State state);
    Event                           add_event(const Event& event);
    void                            add_transition(const Transition& transition);
    void                            add_declaration(const StateDeclaration& decl);
    void                            add_variable(const Variable& var);
    void                            add_import(const Import& imp);
    void                            add_uml_line(const std::string& line);
    static bool                     is_tr_arrow(const std::string& token);

  public:
    Reader(const std::string& filename, bool v);
    ~Reader();

    std::string get_model_name() const;

    size_t      get_uml_line_count() const;
    std::string get_uml_line(size_t i) const;

    size_t    get_variable_count() const;
    Variable* get_variable(size_t id);

    size_t    getPrivateVariableCount() const;
    Variable* getPrivateVariable(size_t id);

    size_t    getPublicVariableCount() const;
    Variable* getPublicVariable(size_t id);

    size_t  getImportCount() const;
    Import* getImport(size_t id);

    size_t getStateCount() const;
    State* getState(size_t id);
    State* getStateById(StateId id);

    size_t getInEventCount() const;
    Event* getInEvent(size_t id);

    size_t getInternalEventCount() const;
    Event* getInternalEvent(size_t id);

    size_t getTimeEventCount() const;
    Event* getTimeEvent(size_t id);

    size_t getOutEventCount() const;
    Event* getOutEvent(size_t id);

    Event* findEvent(const std::string& name);

    size_t      getTransitionCountFromStateId(StateId id) const;
    Transition* getTransitionFrom(StateId id, size_t tr);

    size_t            getDeclCount(StateId stateId, Declaration type) const;
    StateDeclaration* getDeclFromStateId(StateId state_id, Declaration type, size_t id);
};
