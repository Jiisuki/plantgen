/** @file
 *  @brief Generates code from plantuml.
 */

#include <stdint.h>
#include <vector>
#include <iostream>

#include <reader.h>
#include <writer.h>

int main(int argc, char* argv[])
{
    // defaults
    std::string filename;
    std::string outdir = "/mnt/c/repos/plantgen/src-gen";

    /* Parse switches. */
    if (1 < argc)
    {
        filename = argv[1];
    }
    else
    {
        //std::cout << "No filename provided!" << std::endl;
        //return (-1);
        filename = "/mnt/c/repos/plantgen/example/plugin.h";
    }

    // append slash if non-existing on outdir
    if ('/' != outdir.back())
    {
        outdir += "/";
    }

    bool verbose = true;
    bool simple_names = false;
    bool enable_tracing = true;
    bool parent_first = true;

    // Read collection.
    Collection collection;
    Reader_t reader (&collection, filename, verbose);
    reader.compile_collection();
    reader.debug_collection();

    // Configure styler.
    Style styler (&collection, simple_names);

    // Configure writer.
    Writer writer (&collection, &styler, parent_first, enable_tracing, verbose);

    /* generate code */
    writer.generate(filename, outdir);

    return (0);
}

