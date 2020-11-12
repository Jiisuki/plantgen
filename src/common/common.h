//
// Created by Jiisuki on 2020-11-10.
//

#pragma once

#include <string>
#include <vector>
#include <fstream>

struct State
{
    unsigned int id;
    std::string name;
    unsigned int parent;
    bool is_choice;
};

enum class Declaration
{
    Entry,
    Exit,
    OnCycle,
    Comment,
};

struct State_Decl
{
    unsigned int id;
    unsigned int state_id;
    Declaration type;
    std::string content;
};

enum class EventDirection
{
    Incoming,
    Outgoing,
    Internal,
};

struct Event
{
    unsigned int id;
    std::string name;
    bool require_parameter;
    std::string parameter_type;
    bool is_time_event;
    EventDirection direction;
    unsigned int expire_time_ms;
    bool is_periodic;
};

struct Transition
{
    unsigned int id;
    unsigned int state_a;
    unsigned int state_b;
    Event event;
    bool has_guard;
    std::string guard_content;
    std::vector<std::string> action_content;
};

struct Variable
{
    unsigned int id;
    bool is_private;
    std::string name;
    std::string type;
    bool has_specific_initial_value;
    std::string initial_value;
};

struct Import
{
    unsigned int id;
    bool is_global;
    std::string content;
};

class Collection
{
public:
    std::string model_name;
    std::vector<State> states;
    std::vector<Event> events;
    std::vector<Transition> transitions;
    std::vector<State_Decl> state_declarations;
    std::vector<Variable> variables;
    std::vector<Import> imports;
    std::vector<std::string> uml;

    void set_model_name(const std::string& s)
    {
        model_name = s;
        model_name[0] = std::toupper(model_name[0]);
    }

    std::string get_model_name() const
    {
        return (model_name);
    }

    void add_uml_line(const std::string& s)
    {
        uml.push_back(s);
    }

    void stream_uml_lines(std::ofstream& s, const std::string& prepend)
    {
        for (auto l : uml)
        {
            s << prepend << l << std::endl;
        }
    }

    void stream_imports(std::ofstream& s, const std::string& prepend)
    {
        for (auto i : imports)
        {
            s << prepend << "#include ";
            if (i.is_global)
            {
                s << "<" << i.content << ">" << std::endl;
            }
            else
            {
                s << "\"" << i.content << "\"" << std::endl;
            }
        }
    }

    std::vector<Transition> get_transitions_from_state(const State& s) const
    {
        std::vector<Transition> tr;

        for (auto t : transitions)
        {
            if (t.state_a == s.id)
            {
                tr.push_back(t);
            }
        }

        return (tr);
    }

    std::vector<State> find_entry_state(State in) const
    {
        // this function will check if the in state contains an initial sub-state,
        // and follow the path until a state is reached that does not contain an
        // initial sub-state.
        std::vector<State> states;
        states.push_back(in);

        // check if the state contains any initial sub-state
        bool found_next = true;
        while (found_next)
        {
            found_next = false;
            for (auto s : states)
            {
                if ((in.id != s.id) && (in.id == s.parent) && ("initial" == s.name))
                {
                    // a child state was found that is an initial state. Get transition
                    // from this initial state, it should be one and only one.
                    auto transitions = get_transitions_from_state(s);
                    if (transitions.empty())
                    {
                        //errorReport("Initial state in [" + this->styler->getStateName(in) + "] as no transitions.", __LINE__);
                    }
                    else
                    {
                        // get the transition
                        State tmp = get_state_by_id(transitions[0].state_b);
                        if (0 == tmp.id)
                        {
                            //this->errorReport("Initial state in [" + this->styler->getStateName(in) + "] has no target.", __LINE__);
                        }
                        else
                        {
                            // recursively check the state, we can do this by
                            // exiting the for loop but continuing from the top.
                            states.push_back(tmp);
                            in = tmp;

                            // if the state is a choice, we need to stop here.
                            if (tmp.is_choice)
                            {
                                found_next = false;
                            }
                            else
                            {
                                found_next = true;
                            }
                        }
                    }
                }
            }
        }

        return (states);
    }

    std::vector<State> find_path_to_first_state() const
    {
        std::vector<State> path;

        for (auto s : states)
        {
            if ("initial" == s.name)
            {
                // check if this is top initial state
                if (0 == s.parent)
                {
                    // this is the top initial, find transition from this idle
                    std::vector<Transition> transitions = get_transitions_from_state(s);
                    if (transitions.empty())
                    {
                        //this->errorReport("No transition from initial state", __LINE__);
                    }
                    else
                    {
                        // check target state (from top)
                        State trStB = get_state_by_id(transitions[0].state_b);
                        if (0 == trStB.id)
                        {
                            //this->errorReport("Transition to null state", __LINE__);
                        }
                        else
                        {
                            // get state where it stops.
                            path = find_entry_state(trStB);
                        }
                    }
                }
            }
        }

        return (path);
    }

    std::vector<State> find_path_to_final_state(State in) const
    {
        // this function will check if the in state has an outgoing transition to
        // a final state and follow the path until a state is reached that does not
        // contain an initial sub-state.
        std::vector<State> states;
        states.push_back(in);

        // find parent to this state
        bool found_next = true;
        while (found_next)
        {
            found_next = false;
            for (auto s : states)
            {
                if ((in.id != s.id) && (in.parent == s.id) && ("initial" != s.name))
                {
                    // a parent state was found that is not an initial state. Check for
                    // any outgoing transitions from this state to a final state. Store
                    // the id of the tmp state, since we will reuse this pointer.
                    auto transitions = get_transitions_from_state(s);
                    for (auto t : transitions)
                    {
                        State tmp = get_state_by_id(t.state_b);
                        if ((0 != tmp.id) && ("final" == tmp.name))
                        {
                            // recursively check the state
                            states.push_back(tmp);
                            in = tmp;

                            // if this state is a choice, we need to stop there.
                            if (tmp.is_choice)
                            {
                                found_next = false;
                            }
                            else
                            {
                                found_next = true;
                            }
                        }
                    }
                }
            }
        }

        return (states);
    }

    unsigned int get_state_decl_count(const State& s, Declaration decl) const
    {
        unsigned int n = 0;
        for (auto d : state_declarations)
        {
            if ((s.id == d.state_id) && (decl == d.type))
            {
                n++;
            }
        }
        return (n);
    }

    std::vector<State_Decl> get_state_declarations(const State& s) const
    {
        std::vector<State_Decl> decl;
        for (auto d : state_declarations)
        {
            if (s.id == d.state_id)
            {
                decl.push_back(d);
            }
        }
        return (decl);
    }

    std::vector<State_Decl> get_state_declarations(const State& s, Declaration type) const
    {
        std::vector<State_Decl> decl;
        for (auto d : state_declarations)
        {
            if ((s.id == d.state_id) && (type == d.type))
            {
                decl.push_back(d);
            }
        }
        return (decl);
    }

    State get_state_by_id(unsigned int id) const
    {
        State s {};
        for (auto item : states)
        {
            if (id == item.id)
            {
                s = item;
                break;
            }
        }
        return (s);
    }

    bool has_entry_statement(const State& s) const
    {
        if (0 < get_state_decl_count(s, Declaration::Entry))
        {
            return (true);
        }
        else
        {
            std::vector<Transition> transitions = get_transitions_from_state(s);
            for (auto t : transitions)
            {
                if (t.event.is_time_event)
                {
                    return (true);
                }
            }
        }
        return (false);
    }

    bool has_exit_statement(const State& s) const
    {
        if (0 < get_state_decl_count(s, Declaration::Exit))
        {
            return (true);
        }
        else
        {
            std::vector<Transition> transitions = get_transitions_from_state(s);
            for (auto t : transitions)
            {
                if (t.event.is_time_event)
                {
                    return (true);
                }
            }
        }
        return (false);
    }
};

class Style
{
private:
    Collection* collection;
    bool use_simple_names;
    bool use_builtin_time_event_handling;
    std::string delimiter;
    std::string run_cycle_top;
    std::string run_cycle_state;
    std::string enact;
    std::string exact;
    std::string state_base;
    std::string state_type;
    std::string event_raise_base;
    std::string handle_type;
    std::string time_tick;
    std::string time_event;
    std::string is_event_raised_base;
    std::string is_event_raised_end;
    std::string get_variable_base;
    std::string trace_entry;
    std::string trace_exit;
    std::string start_timer;
    std::string stop_timer;

    std::string get_base_decl(const State& state) const
    {
        std::string decl_base = "";

        if (!use_simple_names)
        {
            State parent = collection->get_state_by_id(state.parent);
            if (0 != parent.id)
            {
                decl_base = get_base_decl(parent) + delimiter;
            }
        }

        decl_base += std::toupper(state.name[0]);
        decl_base += state.name.substr(1);

        return (decl_base);
    }

public:
    Style(Collection* collection, bool simple_names)
            : collection(collection)
            , use_simple_names(simple_names)
            , use_builtin_time_event_handling(false)
            , delimiter("_")
            , run_cycle_top("runCycle")
            , run_cycle_state("_react")
            , enact("_entryAction")
            , exact("_exitAction")
            , state_base("State_")
            , state_type("State_t")
            , event_raise_base("raise_")
            , handle_type("Handle_t")
            , time_tick("_timeTick")
            , time_event("timeEvent")
            , is_event_raised_base("is_")
            , is_event_raised_end("_raised")
            , get_variable_base("get_")
            , trace_entry("traceEntry")
            , trace_exit("traceExit")
            , start_timer("startTimer")
            , stop_timer("stopTimer")
    {}

    ~Style() = default;

    bool get_config_use_simple_names() {return use_simple_names;}
    bool get_config_use_builtin_time_event_handling() {return use_builtin_time_event_handling;}

    std::string get_top_run_cycle() const
    {
        return (collection->get_model_name() + delimiter + run_cycle_top);
    }

    std::string get_state_run_cycle(const State& s) const
    {
        return (collection->get_model_name() + delimiter + get_base_decl(s) + run_cycle_state);
    }

    std::string get_state_entry(const State& s) const
    {
        return (collection->get_model_name() + delimiter + get_base_decl(s) + enact);
    }

    std::string get_state_exit(const State& s) const
    {
        return (collection->get_model_name() + delimiter + get_base_decl(s) + exact);
    }

    std::string get_state_name(const State& s) const
    {
        return (collection->get_model_name() + delimiter + state_base + get_base_decl(s));
    }

    std::string get_state_type() const
    {
        return (collection->get_model_name() + delimiter + state_type);
    }

    std::string get_event_raise(const Event& e) const
    {
        return (collection->get_model_name() + delimiter + event_raise_base + e.name);
    }

#if 0
    std::string get_event_raise(const std::string& e) const
    {
        return (collection->get_model_name() + delimiter + event_raise_base + e);
    }
#endif

    std::string get_handle_type() const
    {
        return (collection->get_model_name() + delimiter + handle_type);
    }

    std::string get_time_tick() const
    {
        return (collection->get_model_name() + delimiter + time_tick);
    }

    std::string get_time_event_raise() const
    {
        return (collection->get_model_name() + delimiter + event_raise_base + time_event);
    }

    std::string get_event_is_raised(const Event& e) const
    {
        return (collection->get_model_name() + delimiter + is_event_raised_base + e.name + is_event_raised_end);
    }

    std::string get_variable(const Variable& v) const
    {
        return (collection->get_model_name() + delimiter + get_variable_base + v.name);
    }

    std::string get_trace_entry() const
    {
        return (collection->get_model_name() + delimiter + trace_entry);
    }

    std::string get_trace_exit() const
    {
        return (collection->get_model_name() + delimiter + trace_exit);
    }

    std::string get_start_timer() const
    {
        return (collection->get_model_name() + delimiter + start_timer);
    }

    std::string get_stop_timer() const
    {
        return (collection->get_model_name() + delimiter + stop_timer);
    }
};

