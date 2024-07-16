/** @file
 *  @brief Generates code from plantuml.
 */

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#include "../include/reader.hpp"
#include "../include/writer.hpp"

void configure_default(WriterConfig& cfg, std::string& out)
{
    cfg.use_simple_names = true;
    cfg.verbose = false;
    cfg.do_tracing = false;
    cfg.parent_first_execution = true;
    out = "src/src-gen";
}

void print_usage()
{
    std::cout << "codegen [options]" << std::endl << std::endl;
    std::cout << "\t-h\t\t\tPrint help information" << std::endl;
    std::cout << "\t-l\t\t\tUse long state names" << std::endl;
    std::cout << "\t-v\t\t\tVerbose output" << std::endl;
    std::cout << "\t-t\t\t\tGenerate tracing functions" << std::endl;
    std::cout << "\t-c\t\t\tChild first execution scheme" << std::endl;
    std::cout << "\t-o <folder>\tWhere to store the generated files" << std::endl;
    std::cout << "\t-i <file>\tWhat file to generate" << std::endl << std::endl;
    std::cout << "\tDefault values:" << std::endl;
    std::cout << "\t\tLong state names: disabled" << std::endl;
    std::cout << "\t\tVerbose output:   disabled" << std::endl;
    std::cout << "\t\tGenerate tracing: disabled" << std::endl;
    std::cout << "\t\tChild first exec: disabled" << std::endl;
    std::cout << "\t\tOutput folder:    src/src-gen" << std::endl;
}

int parse_arguments(int argc, char* argv[], WriterConfig& cfg, std::string& in, std::string& out)
{
    for (auto i = 0; i < argc; i++)
    {
        if ('-' == argv[i][0])
        {
            switch (argv[i][1])
            {
                case 'l':
                    cfg.use_simple_names = false;
                    break;

                case 'v':
                    cfg.verbose = true;
                    break;

                case 't':
                    cfg.do_tracing = true;
                    break;

                case 'c':
                    cfg.parent_first_execution = false;
                    break;

                case 'o':
                    if (argc <= (i + 1))
                    {
                        std::cerr << "-o requires <folder>" << std::endl;
                        print_usage();
                        return 1;
                    }
                    else
                    {
                        out = argv[i + 1];
                        i++;
                    }
                    break;

                case 'i':
                    if (argc <= (i + 1))
                    {
                        std::cerr << "-i requires <file>" << std::endl;
                        print_usage();
                        return 1;
                    }
                    else
                    {
                        in = argv[i + 1];
                        i++;
                    }
                    break;

                default:
                    std::cout << "Unknown parameter given: " << argv[i] + 1 << std::endl;
                    print_usage();
                    return 1;

                case 'h':
                    print_usage();
                    return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    WriterConfig cfg {};
    std::string filename {};
    std::string outdir {};

    configure_default(cfg, outdir);
    if (0 != parse_arguments(argc, argv, cfg, filename, outdir))
    {
        return 1;
    }
    else if (filename.empty() || outdir.empty())
    {
        print_usage();
        return 1;
    }

    // append slash if non-existing on outdir
    if ('/' != outdir.back())
    {
        outdir += '/';
    }

    if (!std::filesystem::exists(outdir))
    {
        std::cout << "Creating output directory '" << outdir << "'" << std::endl;
        std::filesystem::create_directories(outdir);
    }

    Writer writer(filename, outdir, cfg);
    writer.generateCode();

    return (0);
}
