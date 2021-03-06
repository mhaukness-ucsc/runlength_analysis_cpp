#include "Identity.hpp"
#include "boost/program_options.hpp"
#include <iostream>
#include <experimental/filesystem>

using std::cout;
using boost::program_options::options_description;
using boost::program_options::variables_map;
using boost::program_options::value;
using std::experimental::filesystem::path;


int main(int argc, char* argv[]){
    path ref_fasta_path;
    path bam_path;
    uint16_t max_threads;

    options_description options("Arguments");

    options.add_options()
        ("ref",
        value<path>(&ref_fasta_path),
        "File path of reference FASTA file containing REFERENCE sequences to be Run-length encoded")

        ("bam",
        value<path>(&bam_path),
        "File path of BAM alignment file containing '--eqx' aligned (!!) sequences")

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

    measure_identity_from_bam(bam_path,
        ref_fasta_path,
        max_threads);

    return 0;
}

