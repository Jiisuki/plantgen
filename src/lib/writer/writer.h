/** @file
 *  @brief Handles writing of the generated code.
 */

#pragma once

#include <common.h>
#include <fstream>

class Writer
{
public:
    explicit Writer(Collection* collection, Style* styler, bool parent_first, bool enable_tracing, bool verbose)
            : collection(collection)
            , styler(styler)
            , parent_first(parent_first)
            , enable_tracing(enable_tracing)
            , verbose(verbose)
    {}

    ~Writer()
    {
        if (out_c.is_open()) {out_c.close();}
        if (out_h.is_open()) {out_h.close();}
    }

    void generate(const std::string& filename, const std::string& outdir);

private:
    Collection* collection;
    Style* styler;
    bool parent_first;
    bool enable_tracing;
    bool verbose;

    std::ofstream out_c;
    std::ofstream out_h;

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
    void proto_traceEntry();
    void proto_traceExit();
    void decl_stateList();
    void decl_eventList();
    void decl_variableList();
    void decl_stateMachine();
    void impl_init(std::vector<State> firstState);
    void impl_raiseInEvent();
    void impl_raiseOutEvent();
    void impl_raiseInternalEvent();
    void impl_checkOutEvent();
    void impl_getVariable();
    void impl_timeTick();
    void impl_clearEvents();
    void impl_topRunCycle();
    void impl_runCycle();
    void impl_entryAction();
    void impl_exitAction();

    std::vector<std::string> tokenize(const std::string& str) const;
    void parse_declaration(const std::string& declaration);
    std::string parse_guard(const std::string& content);
    void parse_choice_path(State& initial_choice, unsigned int indent);
    std::vector<State> get_child_states(const State& current_state);
    bool parse_child_exits(State current_state, unsigned int indent, const State& top_state, const bool did_previous_write);
    std::string get_trace_call_entry(const State& state) const;
    std::string get_trace_call_exit(const State& state) const;
    std::string getIndent(const size_t level);
    std::string get_if_else_if(const unsigned int i);
    void errorReport(std::string str, unsigned int line);

public:

};



