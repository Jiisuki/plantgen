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
    bool verbose = false;
    bool doTracing = false;
    bool isFilenameSet = false;

    /* Parse switches. */
    int i = 1;
    while (i < argc)
    {
        std::string opt = argv[i];

        if ((i+1) < argc)
        {
            std::string val = argv[i+1];

            if ((0 == opt.compare("-v")) || (0 == opt.compare("-V")))
            {
                if (0 != val.compare("0"))
                {
                    verbose = true;
                }
            }
            else if ((0 == opt.compare("-i")) || (0 == opt.compare("-I")))
            {
                filename = val;
                isFilenameSet = true;
            }
            else if ((0 == opt.compare("-o")) || (0 == opt.compare("-O")))
            {
                outdir = val;
            }
            else if ((0 == opt.compare("-t")) || (0 == opt.compare("-T")))
            {
                if (0 != val.compare("0"))
                {
                    doTracing = true;
                }
            }

            // increment 2
            i += 2;
        }
        else
        {
            /* Parameterless switches. */
            if ((0 == opt.compare("-?")) || (0 == opt.compare("--help")))
            {
                helpinfo();
                return (0);
            }
            else
            {
                std::cout << "Invalid parameter '" << opt << "'" << std::endl;
            }

            /* Increment 1. */
            i += 1;
        }
    }

    if (!isFilenameSet)
    {
        ERROR_REPORT(Error, "Require input filename, try codegen -? for help.");
        return (-1);
    }

    const size_t ftype_index = filename.find_last_of(".");

    const std::string modelName = filename.substr(0, ftype_index);

    // append slash if non-existing on outdir
    if ('/' != outdir.back())
    {
        outdir += "/";
    }

    std::string outfile_c = outdir + modelName + ".c";
    std::string outfile_h = outdir + modelName + ".h";

    if (verbose)
    {
        std::cout << "Generating code from '" << filename << "' > '" << outfile_c << "' and '" << outfile_h << "' ..." << std::endl;
    }

    Reader_t* reader = new Reader_t(filename);
    if (NULL == reader)
    {
        ERROR_REPORT(Error, "Allocation error");
        return (-1);
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
    const size_t numEvents = reader->getEventCount();
    const bool isParentFirstExec = true;

    // write header to .h-file
    writer.out_h << "/** @file" << std::endl;
    writer.out_h << " *  @brief Interface to the " << modelName << " state machine." << std::endl;
    writer.out_h << " *" << std::endl;
    // TODO: write ascii art here
    writer.out_h << " */" << std::endl << std::endl;

    // write required headers
    writer.out_h << getIndent(0) << "#include <stdbool.h>" << std::endl << std::endl;

    // write all states to .h
    writer.out_h << "/** @brief State of the state machine. */" << std::endl;
    writer.out_h << getIndent(0) << "typedef enum" << std::endl;
    writer.out_h << getIndent(0) << "{" << std::endl;
    for (size_t i = 0; i < numStates; i++)
    {
        State_t* state = reader->getState(i);
        if (NULL != state)
        {
            writer.out_h << getIndent(1) << getStateName(reader, modelName, state) << "," << std::endl;
        }
    }
    writer.out_h << getIndent(0) << "} " << modelName << "_State_t;" << std::endl << std::endl;

    // write all event types
    writer.out_h << getIndent(0) << "typedef struct" << std::endl;
    writer.out_h << getIndent(0) << "{" << std::endl;
    for (size_t i = 0; i < numEvents; i++)
    {
        Event_t* ev = reader->getEvent(i);
        writer.out_h << getIndent(1) << "bool " << ev->name << ";" << std::endl;
    }
    writer.out_h << getIndent(0) << "} " << modelName << "_EventList_t;" << std::endl;
    writer.out_h << std::endl;

    // write internal structure
    writer.out_h << getIndent(0) << "typedef struct" << std::endl;
    writer.out_h << getIndent(0) << "{" << std::endl;
    writer.out_h << getIndent(1) << modelName << "_State_t state;" << std::endl;
    writer.out_h << getIndent(1) << modelName << "_EventList_t events;" << std::endl;
    writer.out_h << getIndent(0) << "} " << modelName << "_t;" << std::endl << std::endl;

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
    for (size_t i = 0; i < numEvents; i++)
    {
        Event_t* ev = reader->getEvent(i);
        writer.out_h << "/** @brief Raise the " << ev->name << " event in the state machine. */" << std::endl;
        writer.out_h << getIndent(0) << "void " << getEventRaise(reader, modelName, ev) << "(" << getHandleName(modelName) << " handle);" << std::endl << std::endl;
    }

    // write header to .c
    writer.out_c << getIndent(0) << "#include <stdint.h>" << std::endl;
    writer.out_c << getIndent(0) << "#include <stdbool.h>" << std::endl;
    writer.out_c << getIndent(0) << "#include \"" << modelName << ".h\"" << std::endl;
    // TODO: write imports
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
    const State_t* firstState = findInitState(reader, numStates, modelName);

    // write init implementation
    std::cout << "Writing implementation for state machine init..." << std::endl;
    writeImplementation_init(&writer, reader, firstState, modelName);

    // write all raise event functions
    std::cout << "Writing implementations for raising events..." << std::endl;
    writeImplementation_raiseEvent(&writer, reader, numEvents, modelName);

    // write implementation that clears all ingoing events
    std::cout << "Writing implementation for clearing incoming events..." << std::endl;
    writeImplementation_clearInEvents(&writer, reader, numEvents, modelName);

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

