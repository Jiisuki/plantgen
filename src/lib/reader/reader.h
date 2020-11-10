/** @file
 *  @brief Class for reading a plantuml file.
 */

#pragma once

#include <common.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <fstream>

class Reader_t
{
private:
    bool verbose;
    std::ifstream in;
    Collection* collection;

    void collect_states();
    void collect_events();
    std::vector<std::string> tokenize(const std::string& str);
    unsigned int add_state(State& newState);
    Event add_event(Event& newEvent);
    void add_transition(Transition& newTransition);
    void add_declaration(State_Decl& newDecl);
    void add_variable(Variable& newVar);
    void add_import(Import& newImp);
    void add_uml_line(const std::string& line);
    bool is_tr_arrow(const std::string& token);

public:
    Reader_t(Collection* collection, std::string filename, const bool verbose);
    ~Reader_t();
    void compile_collection();
};

