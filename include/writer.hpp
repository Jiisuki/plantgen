/** @file
 *  @brief Handles writing of the generated code.
 */

#pragma once

#include "reader.hpp"
#include "style.hpp"
#include <fstream>

///\brief Configuration for the code generator.
struct WriterConfig
{
    ///\brief Verbose output, mostly for debugging.
    bool verbose;

    ///\brief If true, enable tracing functionality for state entry/exit.
    bool do_tracing;

    ///\brief Use the short state name instead of nested names.
    bool use_simple_names;

    ///\brief Execution scheme, if true, outermost transition is always taken first.
    bool parent_first_execution;

    WriterConfig() : verbose(), do_tracing(), use_simple_names(), parent_first_execution() {}
    ~WriterConfig() = default;
};

class Writer
{
  private:
    WriterConfig config;
    std::string  filename;
    std::string  outdir;
    Reader       reader;
    Style        styler;
    size_t       indent;

    ///\brief Start the namespace tag using the model name as the namespace.
    void start_namespace(std::ofstream& out);

    ///\brief Finishes the namespace.
    void end_namespace(std::ofstream& out);

    ///\brief Write the declaration of the model states.
    void decl_state_list(std::ofstream& out);

    ///\brief Write the declaration of the model events.
    void decl_event_list(std::ofstream& out);

    ///\brief Write the declaration of the model variables.
    void decl_variable_list(std::ofstream& out);

    ///\brief Write the declaration of the tracing functions.
    void decl_tracing_callback(std::ofstream& out);

    ///\brief Write the declaration of the state machine.
    void decl_state_machine(std::ofstream& out);

    ///\brief Write the implementation of the init function.
    void impl_init(std::ofstream& out, const std::vector<State*>& first_state);

    ///\brief Write the implementation of all raise in event functions.
    void impl_raise_in_event(std::ofstream& out);

    ///\brief Write the implementation of all raise out event functions.
    void impl_raise_out_event(std::ofstream& out);

    ///\brief Write the implementation of all raise internal event functions.
    void impl_raise_internal_event(std::ofstream& out);

    void impl_check_out_event(std::ofstream& out);
    void impl_get_variable(std::ofstream& out);
    void impl_time_tick(std::ofstream& out);
    void impl_top_run_cycle(std::ofstream& out);
    void impl_trace_calls(std::ofstream& out);
    void impl_run_cycle(std::ofstream& out);
    void impl_entry_action(std::ofstream& out);
    void impl_exit_action(std::ofstream& out);

    static std::vector<std::string> tokenize(const std::string& str);
    void                            parse_declaration(std::ofstream& out, const std::string& declaration);
    std::string                     parse_guard(const std::string& guardStrRaw);
    void                            parse_choice_path(std::ofstream& out, State* initialChoice);

    std::vector<State*> get_child_states(State* currentState);
    bool parse_child_exits(std::ofstream& out, State* currentState, StateId topState, bool didPreviousWrite);

    bool has_entry_statement(StateId stateId);
    bool has_exit_statement(StateId stateId);

    std::string         get_trace_call_entry(const State* state);
    std::string         get_trace_call_exit(const State* state);
    std::vector<State*> find_init_state();
    std::vector<State*> find_entry_state(State* in);
    std::vector<State*> find_final_state(State* in);
    std::string         get_indent() const;
    static std::string         get_if_else_if(size_t i);

    static void error_report(const std::string& str, unsigned int line);
    void        increase_indent();
    void        decrease_indent();
    void        reset_indent();

  public:
    Writer(const std::string& filename, const std::string& outdir, const WriterConfig& cfg);
    ~Writer() = default;
    void generateCode();
};
