/** @file
 *  @brief Generates code from plantuml.
 */

#include <stdint.h>
#include <vector>
#include <iostream>

#include <reader.hpp>
#include <writer.hpp>

int main(int argc, char* argv[])
{
    // defaults
    std::string filename;
    std::string outdir = "src/src-gen";

    /* Parse switches. */
    if (1 < argc)
    {
        filename = argv[1];
    }
    else
    {
        std::cout << "No filename provided!" << std::endl;
        return (-1);
    }

    // append slash if non-existing on outdir
    if ('/' != outdir.back())
    {
        outdir += "/";
    }

    Writer_t* writer = new Writer_t();

    /* configure writer */
    writer->enableSimpleNames();
    writer->enableVerbose();
    writer->enableParentFirstExecution();
    writer->enableTracing();

    /* generate code */
    writer->generateCode(filename, outdir);

    /* cleanup */
    delete writer;

    return (0);
}

