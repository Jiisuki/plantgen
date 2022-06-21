/** @file
 *  @brief Generates code from plantuml.
 */

#include <cstdint>
#include <vector>
#include <iostream>
#include <filesystem>

#include "../include/reader.hpp"
#include "../include/writer.hpp"

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

    if (!std::filesystem::exists(outdir))
    {
        std::cout << "Creating output directory '" << outdir << "'" << std::endl;
        std::filesystem::create_directories(outdir);
    }

    /* configure writer */
    Writer_Config_t cfg {};
    cfg.useSimpleNames = true;
    cfg.verbose = true;
    cfg.doTracing = true;
    cfg.parentFirstExecution = true;

    Writer_t writer (filename, outdir, cfg);

    /* generate code */
    writer.generateCode();

    return (0);
}

