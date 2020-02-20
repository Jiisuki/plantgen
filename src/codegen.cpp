/** @file
 *  @brief Generates code from plantuml.
 */

#include <stdint.h>
#include <vector>
#include <iostream>

#include <reader.hpp>
#include <helper.hpp>
#include <writer.hpp>

void helpinfo(void)
{
    std::cout << "Commandline switches:" << std::endl;
    std::cout << getIndent(1) << "-v [0/1]  : Disable/Enable verbose output." << std::endl;
    std::cout << getIndent(1) << "-t [0/1]  : Disable/Enable model tracing output." << std::endl;
    std::cout << getIndent(1) << "-i [FILE] : Specify input filename (required)." << std::endl;
    std::cout << getIndent(1) << "-o [DIR]  : Specify output directory." << std::endl;
}

int main(int argc, char* argv[])
{
    // defaults
    std::string filename;
    std::string outdir = "src-gen/src";
    bool verbose = true;
    bool doTracing = false;

    /* Parse switches. */
    if (1 < argc)
    {
        filename = argv[1];
    }
    else
    {
        ERROR_REPORT(Error, "No filename provided!");
        return (-1);
    }

    // append slash if non-existing on outdir
    if ('/' != outdir.back())
    {
        outdir += "/";
    }

    Reader_t* reader = new Reader_t(filename, verbose);
    if (NULL == reader)
    {
        ERROR_REPORT(Error, "Allocation error");
        return (-1);
    }

    const std::string modelName = reader->getModelName();
    std::string outfile_c = outdir + modelName + ".c";
    std::string outfile_h = outdir + modelName + ".h";

    if (verbose)
    {
        std::cout << "Generating code from '" << filename << "' > '" << outfile_c << "' and '" << outfile_h << "' ..." << std::endl;
    }

    Writer_t writer;
    writer.out_c.open(outfile_c);
    writer.out_h.open(outfile_h);

    if (!writer.out_c.is_open() || !writer.out_h.is_open())
    {
        ERROR_REPORT(Error, "Failed to open output files, does directory exist?");
        return (-1);
    }

    Writer_setTracing(doTracing);

    const size_t numStates = reader->getStateCount();
    const size_t numInEvents = reader->getInEventCount();
    const size_t numOutEvents = reader->getOutEventCount();
    const size_t numVariables = reader->getVariableCount();
    const size_t numImports = reader->getImportCount();
    const size_t numUmlLines = reader->getUmlLineCount();

    const bool isParentFirstExec = true;

    // write header to .h-file
    writer.out_h << "/** @file" << std::endl;
    writer.out_h << " *  @brief Interface to the " << modelName << " state machine." << std::endl;
    writer.out_h << " *" << std::endl;
    writer.out_h << " *  @startuml" << std::endl;
    for (size_t i = 0; i < numUmlLines; i++)
    {
        writer.out_h << " *  " << reader->getUmlLine(i) << std::endl;
    }
    writer.out_h << " *  @enduml" << std::endl;
    writer.out_h << " */" << std::endl << std::endl;

    // write required headers
    writer.out_h << getIndent(0) << "#include <stdint.h>" << std::endl;
    writer.out_h << getIndent(0) << "#include <stdbool.h>" << std::endl;
    for (size_t i = 0; i < numImports; i++)
    {
        Import_t* imp = reader->getImport(i);
        writer.out_h << getIndent(0) << "#include ";
        if (imp->isGlobal)
        {
            writer.out_h << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            writer.out_h << "\"" << imp->name << "\"" << std::endl;
        }
    }
    writer.out_h << std::endl;

    // write all states to .h
    writeDeclaration_stateList(&writer, reader, numStates, modelName);

    // write all event types
    writeDeclaration_eventList(&writer, reader, numInEvents, modelName);

    // write all variables
    writeDeclaration_variableList(&writer, reader, numVariables, modelName);

    // write main declaration
    writeDeclaration_stateMachine(&writer, reader, modelName);

    // write handle
    writer.out_h << "typedef " << modelName << "_t* " << getHandleName(modelName) << ";" << std::endl << std::endl;

    // write required implementation
    if (doTracing)
    {
        writer.out_h << "/* The state machine uses tracing, therefore the following functions" << std::endl;
        writer.out_h << " * are required to be implemented by the user. */" << std::endl;
        writePrototype_traceEntry(&writer, modelName);
        writePrototype_traceExit(&writer, modelName);
        writer.out_h << std::endl;
    }

    // write init function
    writer.out_h << "/** @brief Initialises the state machine. */" << std::endl;
    writer.out_h << "void " << modelName << "_init(" << getHandleName(modelName) << " handle);" << std::endl << std::endl;

    // write all raise event function prototypes
    if (verbose)
    {
        std::cout << "Writing raise in event prototypes..." << std::endl;
    }
    writePrototype_raiseInEvent(&writer, reader, numInEvents, modelName);

    // write all get raised events prototypes
    // TODO
    (void) numOutEvents;

    // write header to .c
    writer.out_c << getIndent(0) << "#include <stdint.h>" << std::endl;
    writer.out_c << getIndent(0) << "#include <stdbool.h>" << std::endl;
    writer.out_c << getIndent(0) << "#include \"" << modelName << ".h\"" << std::endl;
    // TODO: write imports
    for (size_t i = 0; i < reader->getImportCount(); i++)
    {
        Import_t* imp = reader->getImport(i);
        writer.out_c << getIndent(0) << "#include ";
        if (imp->isGlobal)
        {
            writer.out_c << "<" << imp->name << ">" << std::endl;
        }
        else
        {
            writer.out_c << "\"" << imp->name << "\"" << std::endl;
        }
    }
    writer.out_c << std::endl;


    // declare prototypes for global run cycle, clear in events.
    writer.out_c << "static void runCycle(" << getHandleName(modelName) << " handle);" << std::endl;
    writer.out_c << "static void clearInEvents(" << getHandleName(modelName) << " handle);" << std::endl;
    writer.out_c << std::endl;

    // declare prototypes for run cycle
    std::cout << "Writing prototypes for state run cycles..." << std::endl;
    writePrototype_runCycle(&writer, reader, numStates, modelName);

    // declare prototypes for react
    std::cout << "Writing prototypes for state reactions..." << std::endl;
    writePrototype_react(&writer, reader, numStates, modelName);

    // declare prototypes for entry actions
    std::cout << "Writing prototypes for state entry actions..." << std::endl;
    writePrototype_entryAction(&writer, reader, numStates, modelName);

    // declare prototypes for exit actions
    std::cout << "Writing prototypes for state exit actions..." << std::endl;
    writePrototype_exitAction(&writer, reader, numStates, modelName);

    // find first state on init
    std::cout << "Searching for initial state..." << std::endl;
    const std::vector<State_t*> firstState = findInitState(reader, numStates, modelName);

    // write init implementation
    std::cout << "Writing implementation for state machine init..." << std::endl;
    writeImplementation_init(&writer, reader, firstState, modelName);

    // write all raise event functions
    std::cout << "Writing implementations for raising events..." << std::endl;
    writeImplementation_raiseInEvent(&writer, reader, numInEvents, modelName);

    // write implementation that clears all ingoing events
    std::cout << "Writing implementation for clearing incoming events..." << std::endl;
    writeImplementation_clearInEvents(&writer, reader, numInEvents, modelName);

    // write implementation of the topmost run cycle handler
    std::cout << "Writing implementation for top run cycle..." << std::endl;
    writeImplementation_topRunCycle(&writer, reader, numStates, modelName);

    // write run cycle functions
    std::cout << "Writing implementation for all state run cycles..." << std::endl;
    writeImplementation_runCycle(&writer, reader, numStates, isParentFirstExec, modelName);

    // write state reactions
    std::cout << "Writing parent state reactions..." << std::endl;
    writeImplementation_reactions(&writer, reader, numStates, modelName);

    // write state entry/exit/oncycle actions
    std::cout << "Writing state actions..." << std::endl;
    writeImplementation_entryAction(&writer, reader, numStates, modelName);
    writeImplementation_exitAction(&writer, reader, numStates, modelName);
    writeImplementation_oncycleAction(&writer, reader, numStates, modelName);

    /* end with a new line. */
    writer.out_c << std::endl;
    writer.out_h << std::endl;

    /* close streams. */
    writer.out_c.close();
    writer.out_h.close();

    /* free resource. */
    delete reader;

    return (0);
}

