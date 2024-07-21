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
#ifdef DEFAULT_LONG_NAMES
    cfg.use_simple_names = false;
#else
    cfg.use_simple_names = true;
#endif

#ifdef DEFAULT_VERBOSE
    cfg.verbose = true;
#else
    cfg.verbose = false;
#endif

#ifdef DEFAULT_TRACING
    cfg.do_tracing = true;
#else
    cfg.do_tracing = false;
#endif

#ifdef DEFAULT_CHILD_FIRST
    cfg.parent_first_execution = false;
#else
    cfg.parent_first_execution = true;
#endif

#ifdef DEFAULT_OUTPUT
    out = DEFAULT_OUTPUT;
#else
    out = "src/src-gen";
#endif
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
#ifdef DEFAULT_LONG_NAMES
    std::cout << "\t\tLong state names: enabled" << std::endl;
#else
    std::cout << "\t\tLong state names: disabled" << std::endl;
#endif
#ifdef DEFAULT_VERBOSE
    std::cout << "\t\tVerbose output:   enabled" << std::endl;
#else
    std::cout << "\t\tVerbose output:   disabled" << std::endl;
#endif
#ifdef DEFAULT_TRACING
    std::cout << "\t\tGenerate tracing: enabled" << std::endl;
#else
    std::cout << "\t\tGenerate tracing: disabled" << std::endl;
#endif
#ifdef DEFAULT_CHILD_FIRST
    std::cout << "\t\tChild first exec: enabled" << std::endl;
#else
    std::cout << "\t\tChild first exec: disabled" << std::endl;
#endif
#ifdef DEFAULT_OUTPUT
    std::cout << "\t\tOutput folder:    " << DEFAULT_OUTPUT << std::endl;
#else
    std::cout << "\t\tOutput folder:    src/src-gen" << std::endl;
#endif
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
    std::string  filename {};
    std::string  outdir {};

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
