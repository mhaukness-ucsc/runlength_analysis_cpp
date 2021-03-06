#include "Identity.hpp"
#include "boost/program_options.hpp"
#include <iostream>
#include <string>
#include <experimental/filesystem>

using std::cout;
using std::string;
using boost::program_options::options_description;
using boost::program_options::variables_map;
using boost::program_options::value;
using std::experimental::filesystem::path;


int main(int argc, char* argv[]){
    path ref_fasta_path;
    path reads_fasta_path;
    path output_dir;
    string minimap_preset;
    uint16_t max_threads;

    options_description options("Arguments");

    options.add_options()
        ("ref",
        value<path>(&ref_fasta_path),
        "File path of reference FASTA file containing REFERENCE sequences to be Run-length encoded")

        ("sequences",
        value<path>(&reads_fasta_path),
        "File path of FASTA file containing QUERY sequences to be Run-length encoded")

        ("minimap_preset",
        value<string>(&minimap_preset),
        "Minimap preset to be used, e.g. asm20, map-ont")

        ("output_dir",
        value<path>(&output_dir)->
        default_value("output/"),
        "Destination directory. File will be named based on input file name")

        ("max_threads",
        value<uint16_t>(&max_threads)->
        default_value(1),
        "Maximum number of threads to launch");

    // Store options in a map and apply values to each corresponding variable
    variables_map vm;
    store(parse_command_line(argc, argv, options), vm);
    notify(vm);

    // If help was specified, or no arguments given, provide help
    if (vm.count("help") || argc == 1) {
        cout << options << "\n";
        return 0;
    }

    measure_identity_from_fasta(reads_fasta_path,
                                ref_fasta_path,
                                output_dir,
                                minimap_preset,
                                max_threads);

    return 0;
}

