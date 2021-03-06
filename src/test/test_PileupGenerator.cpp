
#include "BinaryRunnieWriter.hpp"
#include "BinaryRunnieReader.hpp"
#include "RunlengthReader.hpp"
#include "FastaWriter.hpp"
#include "PileupGenerator.hpp"
#include "RunlengthWriter.hpp"
#include "SequenceElement.hpp"
#include "FastaReader.hpp"
#include "Align.hpp"

using std::cerr;
using std::ifstream;


void test_standard_pileup(path project_directory){
//    // Get test BAM path
//    path relative_bam_path = "/data/test/test_alignable_sequences_non_RLE_VS_test_alignable_reference_non_RLE.sorted.bam";
//    path absolute_bam_path = project_directory / relative_bam_path;

    // Get test FASTA reads path
    path relative_fasta_reads_path = "/data/test/test_alignable_sequences_non_RLE.fasta";
    path absolute_fasta_reads_path = project_directory / relative_fasta_reads_path;

    // Get test FASTA reference path
    path relative_fasta_ref_path = "/data/test/test_alignable_reference_non_RLE.fasta";
    path absolute_fasta_ref_path = project_directory / relative_fasta_ref_path;

        // Setup Alignment parameters
    bool sort = true;
    bool index = true;
    bool delete_intermediates = false;
    uint16_t k = 19;
    string minimap_preset = "asm20";
    bool explicit_mismatch = true;
    path output_directory = project_directory / "/data/test/";
    size_t max_threads = 30;

    // Align reads to the reference
    path bam_path;
    bam_path = align(
            absolute_fasta_ref_path,
            absolute_fasta_reads_path,
            output_directory,
            sort,
            index,
            delete_intermediates,
            k,
            minimap_preset,
            explicit_mismatch,
            max_threads);

    path absolute_bam_path = absolute(bam_path);

    PileupGenerator pileup_generator = PileupGenerator(absolute_bam_path, 20);

    Region region = Region("synthetic_ref_0", 0, 1337);

    FastaReader ref_reader = FastaReader(absolute_fasta_ref_path);
    FastaReader sequence_reader = FastaReader(absolute_fasta_reads_path);

    Pileup pileup;

    pileup_generator.fetch_region(region, sequence_reader, pileup);
    pileup_generator.print(pileup);


    SequenceElement sequence;
    vector <tuple <string,int64_t,int64_t> > read_indexes;
    pileup_generator.fetch_sequence_indexes_from_region(region, read_indexes);
    int i = 0;
    for (auto& index: read_indexes){
        string name = std::get<0>(index);
//        int64_t start = std::get<1>(index);
//        int64_t stop = std::get<2>(index);

        sequence_reader.get_sequence(sequence, name);

        cout << '>' << sequence.name << '\n';
        cout << sequence.sequence << '\n';
        i++;
    }


}


void write_fasta_as_runlength(
        path absolute_fasta_reads_path,
        path absolute_fasta_ref_path,
        path absolute_runlength_fasta_reads_path,
        path absolute_runlength_fasta_ref_path,
        path absolute_runlength_reads_path,
        path absolute_runlength_ref_path){

    SequenceElement sequence;
    RunlengthSequenceElement runlength_sequence;

    cerr << "READING: " << absolute_fasta_reads_path << '\n';
    cerr << "READING: " << absolute_fasta_ref_path << '\n';
    FastaReader reads_fasta_reader(absolute_fasta_reads_path);
    FastaReader ref_fasta_reader(absolute_fasta_ref_path);
    reads_fasta_reader.index();

    cerr << "WRITING: " << absolute_runlength_fasta_reads_path << '\n';
    cerr << "WRITING: " << absolute_runlength_fasta_ref_path << '\n';
    FastaWriter reads_fasta_writer(absolute_runlength_fasta_reads_path);
    FastaWriter ref_fasta_writer(absolute_runlength_fasta_ref_path);

    cerr << "WRITING: " << absolute_runlength_reads_path << '\n';
    cerr << "WRITING: " << absolute_runlength_ref_path << '\n';
    RunlengthWriter reads_writer(absolute_runlength_reads_path);
    RunlengthWriter ref_writer(absolute_runlength_ref_path);

    while (not reads_fasta_reader.end_of_file) {
        reads_fasta_reader.next_element(sequence);
        runlength_encode(runlength_sequence, sequence);
        reads_fasta_writer.write(runlength_sequence);
        reads_writer.write_sequence(runlength_sequence);
    }
    reads_writer.write_indexes();

    while (not ref_fasta_reader.end_of_file) {
        ref_fasta_reader.next_element(sequence);
        runlength_encode(runlength_sequence, sequence);
        ref_fasta_writer.write(runlength_sequence);
        ref_writer.write_sequence(runlength_sequence);
    }
    ref_writer.write_indexes();
}


void write_fasta_as_runnie(
        path absolute_fasta_reads_path,
        path absolute_fasta_ref_path,
        path absolute_runlength_fasta_reads_path,
        path absolute_runlength_fasta_ref_path,
        path absolute_runlength_reads_path,
        path absolute_runlength_ref_path){

    SequenceElement sequence;
    RunlengthSequenceElement runlength_sequence;
    RunnieSequenceElement runnie_sequence;

    cerr << "READING: " << absolute_fasta_reads_path << '\n';
    cerr << "READING: " << absolute_fasta_ref_path << '\n';
    FastaReader reads_fasta_reader(absolute_fasta_reads_path);
    FastaReader ref_fasta_reader(absolute_fasta_ref_path);
    reads_fasta_reader.index();

    cerr << "WRITING: " << absolute_runlength_fasta_reads_path << '\n';
    cerr << "WRITING: " << absolute_runlength_fasta_ref_path << '\n';
    FastaWriter reads_fasta_writer(absolute_runlength_fasta_reads_path);
    FastaWriter ref_fasta_writer(absolute_runlength_fasta_ref_path);

    cerr << "WRITING: " << absolute_runlength_reads_path << '\n';
    cerr << "WRITING: " << absolute_runlength_ref_path << '\n';
    BinaryRunnieWriter reads_writer(absolute_runlength_reads_path);
    BinaryRunnieWriter ref_writer(absolute_runlength_ref_path);

    while (not reads_fasta_reader.end_of_file) {
        reads_fasta_reader.next_element(sequence);
        runlength_encode(runlength_sequence, sequence);
        reads_fasta_writer.write(runlength_sequence);

        // Copy the runlength sequence trivially to the runnie sequence s.t. each scale/shape is based on the length
        runnie_sequence.name = runlength_sequence.name;
        runnie_sequence.sequence = runlength_sequence.sequence;
        for (auto& length:runlength_sequence.lengths){
            runnie_sequence.scales.emplace_back(length);
            runnie_sequence.shapes.emplace_back(length + 0.5);
        }

        reads_writer.write_sequence(runnie_sequence);
    }
    reads_writer.write_indexes();

    while (not ref_fasta_reader.end_of_file) {
        ref_fasta_reader.next_element(sequence);
        runlength_encode(runlength_sequence, sequence);
        ref_fasta_writer.write(runlength_sequence);

        // Copy the runlength sequence trivially to the runnie sequence s.t. each scale/shape is based on the length
        runnie_sequence.name = runlength_sequence.name;
        runnie_sequence.sequence = runlength_sequence.sequence;
        for (auto& length:runlength_sequence.lengths){
            runnie_sequence.scales.emplace_back(length);
            runnie_sequence.shapes.emplace_back(length + 0.5);
        }

        ref_writer.write_sequence(runnie_sequence);
    }
    ref_writer.write_indexes();
}


void test_runnie_pileup(path project_directory) {
    // Get test FASTA reads path
    path relative_fasta_reads_path = "/data/test/test_alignable_sequences_non_RLE.fasta";
    path absolute_fasta_reads_path = project_directory / relative_fasta_reads_path;

    // Get test FASTA reference path
    path relative_fasta_ref_path = "/data/test/test_alignable_reference_non_RLE.fasta";
    path absolute_fasta_ref_path = project_directory / relative_fasta_ref_path;

    // Create filenames for runlength files
    path absolute_runlength_reads_path = absolute_fasta_reads_path;
    absolute_runlength_reads_path.replace_extension(".rlq");
    path absolute_runlength_ref_path = absolute_fasta_ref_path;
    absolute_runlength_ref_path.replace_extension(".rlq");

    // Create filenames for runlength FASTA files.........
    path absolute_runlength_fasta_reads_path = absolute_fasta_reads_path;
    absolute_runlength_fasta_reads_path.replace_extension("rle.fasta");
    path absolute_runlength_fasta_ref_path = absolute_fasta_ref_path;
    absolute_runlength_fasta_ref_path.replace_extension("rle.fasta");

    write_fasta_as_runnie(
        absolute_fasta_reads_path,
        absolute_fasta_ref_path,
        absolute_runlength_fasta_reads_path,
        absolute_runlength_fasta_ref_path,
        absolute_runlength_reads_path,
        absolute_runlength_ref_path);

    // Setup Alignment parameters
    bool sort = true;
    bool index = true;
    bool delete_intermediates = false;
    uint16_t k = 19;
    string minimap_preset = "asm20";
    bool explicit_mismatch = true;
    path output_directory = project_directory / "/data/test/";
    size_t max_threads = 30;

    // Align reads to the reference
    path bam_path;
    bam_path = align(
            absolute_runlength_fasta_ref_path,
            absolute_runlength_fasta_reads_path,
            output_directory,
            sort,
            index,
            delete_intermediates,
            k,
            minimap_preset,
            explicit_mismatch,
            max_threads);

    path absolute_bam_path = absolute(bam_path);

    PileupGenerator pileup_generator = PileupGenerator(absolute_bam_path, 20);

    Region region = Region("synthetic_ref_0", 0, 1268);

    ifstream check(absolute_runlength_reads_path);
    cerr << check.good() << '\n';

    BinaryRunnieReader reads_runnie_reader(absolute_runlength_reads_path);
    BinaryRunnieReader ref_runnie_reader(absolute_runlength_ref_path);

    Pileup pileup;

    pileup_generator.fetch_region(region, reads_runnie_reader, pileup);
    pileup_generator.print(pileup);
}


void test_runlength_pileup(path project_directory) {
    // Get test FASTA reads path
    path relative_fasta_reads_path = "/data/test/test_alignable_sequences_non_RLE.fasta";
    path absolute_fasta_reads_path = project_directory / relative_fasta_reads_path;

    // Get test FASTA reference path
    path relative_fasta_ref_path = "/data/test/test_alignable_reference_non_RLE.fasta";
    path absolute_fasta_ref_path = project_directory / relative_fasta_ref_path;

    // Create filenames for runlength files
    path absolute_runlength_reads_path = absolute_fasta_reads_path;
    absolute_runlength_reads_path.replace_extension(".rlq");
    path absolute_runlength_ref_path = absolute_fasta_ref_path;
    absolute_runlength_ref_path.replace_extension(".rlq");

    // Create filenames for runlength FASTA files.........
    path absolute_runlength_fasta_reads_path = absolute_fasta_reads_path;
    absolute_runlength_fasta_reads_path.replace_extension("rle.fasta");
    path absolute_runlength_fasta_ref_path = absolute_fasta_ref_path;
    absolute_runlength_fasta_ref_path.replace_extension("rle.fasta");

    write_fasta_as_runlength(
        absolute_fasta_reads_path,
        absolute_fasta_ref_path,
        absolute_runlength_fasta_reads_path,
        absolute_runlength_fasta_ref_path,
        absolute_runlength_reads_path,
        absolute_runlength_ref_path);

    // Setup Alignment parameters
    bool sort = true;
    bool index = true;
    bool delete_intermediates = false;
    uint16_t k = 19;
    string minimap_preset = "asm20";
    bool explicit_mismatch = true;
    path output_directory = project_directory / "/data/test/";
    size_t max_threads = 30;

    // Align reads to the reference
    path bam_path;
    bam_path = align(
            absolute_runlength_fasta_ref_path,
            absolute_runlength_fasta_reads_path,
            output_directory,
            sort,
            index,
            delete_intermediates,
            k,
            minimap_preset,
            explicit_mismatch,
            max_threads);

    path absolute_bam_path = absolute(bam_path);

    PileupGenerator pileup_generator = PileupGenerator(absolute_bam_path, 20);

    string ref_name = "synthetic_ref_0";
    Region region = Region(ref_name, 0, 1268);

    ifstream check(absolute_runlength_reads_path);
    cerr << check.good() << '\n';

    RunlengthReader reads_runlength_reader(absolute_runlength_reads_path);
    RunlengthReader ref_runlength_reader(absolute_runlength_ref_path);

    Pileup pileup;
    Pileup ref_pileup;

    pileup_generator.fetch_region(region, reads_runlength_reader, pileup);
    pileup_generator.generate_reference_pileup(pileup, ref_pileup, region, ref_runlength_reader);
    pileup_generator.print(ref_pileup);
    pileup_generator.print(pileup);
}


void test_runlength_pileup_inserts(path project_directory) {
    // Get test FASTA reads path
    path relative_fasta_reads_input_path = "/data/test/test_pileup_inserts.fasta";
    path absolute_fasta_reads_input_path = project_directory / relative_fasta_reads_input_path;

    // Get test FASTA reference path
    path relative_fasta_ref_input_path = "/data/test/test_pileup_inserts_ref.fasta";
    path absolute_fasta_ref_input_path = project_directory / relative_fasta_ref_input_path;

    // Get test FASTA reads path
    path relative_fasta_reads_path = "/data/test/test_pileup_inserts_appended.fasta";
    path absolute_fasta_reads_path = project_directory / relative_fasta_reads_path;

    // Get test FASTA reference path
    path relative_fasta_ref_path = "/data/test/test_pileup_inserts_ref_appended.fasta";
    path absolute_fasta_ref_path = project_directory / relative_fasta_ref_path;

    // Create filenames for runlength files
    path absolute_runlength_reads_path = absolute_fasta_reads_path;
    absolute_runlength_reads_path.replace_extension(".rlq");
    path absolute_runlength_ref_path = absolute_fasta_ref_path;
    absolute_runlength_ref_path.replace_extension(".rlq");

    // Create filenames for runlength FASTA files.........
    path absolute_runlength_fasta_reads_path = absolute_fasta_reads_path;
    absolute_runlength_fasta_reads_path.replace_extension("rle.fasta");
    path absolute_runlength_fasta_ref_path = absolute_fasta_ref_path;
    absolute_runlength_fasta_ref_path.replace_extension("rle.fasta");

    FastaReader reads_fasta_reader(absolute_fasta_reads_input_path);
    FastaWriter reads_fasta_writer(absolute_fasta_reads_path);
    FastaReader ref_fasta_reader(absolute_fasta_ref_input_path);
    FastaWriter ref_fasta_writer(absolute_fasta_ref_path);

    string prefix = "ACCTTGCGATGCTAGCATAGCATGGCTCATGAATGCGATCCGATTGCAGTCCGAATGCATTGCTACGCAGTGCATAGGCTAGCTCAGACTGCTAGCTAGGCTACTAGCATGCCTAGTCAGTGACTAGCCTAGCGTTAGTCG";
    string suffix = "ATCATATCAGCGTACTCATCGATGCAGCACATGCATGCTATGTCTAGTACTACGGTACGATTATTCGATTCGCCGATGATCTAGCATGCGTACTGCTAGATGCTATGCATGCGTATGATATCTGATGCATGTCAGTTATGCATATTCGATATGTACTAGTTGCAGTCATGTGCATTATGCAGCTATTATTACGCTGAGTGCATAGCATGTCTGTCGCTAGCTAGATCGTAGCATGATCAGCATCATTCATGCATTCGTAGCATGCTAATGTCATTATAGCTATCGGCATTATCAGTAGCATCTAGCATAGATCTCAGTACGTAGTATCTATCAGTCAGTAGCTAGTCGATACGTATCGTTGCATATGCTACGATGATGCTATGCATGGCAATGCATTGCACTAGCTACGTATACTATGCATGTCAGTAGATGCTATACTGATCGAATCTTGATCTAGTAGCCTAGTAGCACTGACTGCATGTCAGTACGTAGCTACGTTCGTATACGATCATCTAGATCATGCTAGCATGCATGCATATATGTGACTGATGCTGATGCCGGATTATCGCGTATCGATCGATCATCATGATCATGATGATGCTGCATCAGAATACGCTGACTGACATCGACGACTGCATCGCGACTGCATCGGCAGCTAGCATGCGCGATGCATGCATCGTACTGCATGCAGTCGATCGATGCACGATGCATGCATGCATCGAGTATAGCCGGATTAGCTACTGAGCGATTTATCTCTGAGGAGATCTCGATCGTAGCATGCTGCGCATCTGCTAATGTCGGATGCTAGCGCTAGCTGCTTAGCTCATATTACGTATCTGATCTGATTCGATGCATGCATTATCGATTCGTATTAGCATCGTACGTAGCTATGCATTCGTAGCTAGCATCGTAGCTGAGCGATGCTATGCGCTAGCTTAGCGATGCTGCCGATCGTAGCGTATCAGAGTCGATCGTAGCTAGCTACGCGTACTAGCTAGCTACGTTAGCGCTAGATGATCTAGGCGCTATTATCGAGAGTCTCTAGGCTACTGATATCTGAGCAGGAAGAGTCGATCGTATGCTGCTGCTAGTCGTACGTATCGTATCGATGCATGTCATGCATAGTATGCGATCGCATGCTACTGTGCTGATGCTAGCTAGCTAGTCGATGTCGTAGCGGCATGTAGCGTACGCGG";

    SequenceElement sequence;
    while (not reads_fasta_reader.end_of_file) {
        reads_fasta_reader.next_element(sequence);
        sequence.sequence = prefix + sequence.sequence + suffix;
        reads_fasta_writer.write(sequence);
    }
    while (not ref_fasta_reader.end_of_file) {
        ref_fasta_reader.next_element(sequence);
        sequence.sequence = prefix + sequence.sequence + suffix;
        ref_fasta_writer.write(sequence);
    }

    write_fasta_as_runlength(
        absolute_fasta_reads_path,
        absolute_fasta_ref_path,
        absolute_runlength_fasta_reads_path,
        absolute_runlength_fasta_ref_path,
        absolute_runlength_reads_path,
        absolute_runlength_ref_path);

    // Setup Alignment parameters
    bool sort = true;
    bool index = true;
    bool delete_intermediates = false;
    uint16_t k = 19;
    string minimap_preset = "asm20";
    bool explicit_mismatch = true;
    path output_directory = project_directory / "/data/test/";
    size_t max_threads = 30;

    // Align reads to the reference
    path bam_path;
    bam_path = align(
            absolute_runlength_fasta_ref_path,
            absolute_runlength_fasta_reads_path,
            output_directory,
            sort,
            index,
            delete_intermediates,
            k,
            minimap_preset,
            explicit_mismatch,
            max_threads);

    path absolute_bam_path = absolute(bam_path);

    PileupGenerator pileup_generator = PileupGenerator(absolute_bam_path, 80);

    string ref_name = "ref";
    Region region = Region(ref_name, 5, 600);

    ifstream check(absolute_runlength_reads_path);
    cerr << check.good() << '\n';

    RunlengthReader reads_runlength_reader(absolute_runlength_reads_path);
    RunlengthReader ref_runlength_reader(absolute_runlength_ref_path);

    Pileup pileup;
    Pileup ref_pileup;

    pileup_generator.fetch_region(region, reads_runlength_reader, pileup);
    pileup_generator.generate_reference_pileup(pileup, ref_pileup, region, ref_runlength_reader);
    pileup_generator.print(ref_pileup);
    pileup_generator.print(pileup);

    RunlengthSequenceElement runlength_sequence;
    vector <tuple <string,int64_t,int64_t> > read_indexes;
    pileup_generator.fetch_sequence_indexes_from_region(region, read_indexes);
    int i = 0;
    for (auto& index: read_indexes){
        string name = std::get<0>(index);
        int64_t start = std::get<1>(index);
        int64_t stop = std::get<2>(index);

        reads_runlength_reader.get_sequence(runlength_sequence, name);

        runlength_sequence.sequence = runlength_sequence.sequence.substr(start, stop-start);
        runlength_sequence.lengths = std::vector<uint16_t>(runlength_sequence.lengths.begin() + start, runlength_sequence.lengths.begin() + stop);

        cout << runlength_sequence.sequence << '\n';
        i++;
    }
}


int main(){
    path script_path = __FILE__;
    path project_directory = script_path.parent_path().parent_path().parent_path();

    test_standard_pileup(project_directory);
    test_runlength_pileup(project_directory);
    test_runnie_pileup(project_directory);

    test_runlength_pileup_inserts(project_directory);

    return 0;
}
