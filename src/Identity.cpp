#include "Identity.hpp"
#include "AlignedSegment.hpp"
#include "FastaReader.hpp"
#include "BamReader.hpp"
#include "Align.hpp"
#include <vector>
#include <thread>
#include <string>
#include <iostream>
#include <mutex>
#include <exception>
#include <atomic>
#include <experimental/filesystem>

using std::vector;
using std::string;
using std::to_string;
using std::make_pair;
using std::thread;
using std::cout;
using std::cerr;
using std::flush;
using std::mutex;
using std::lock_guard;
using std::thread;
using std::ref;
using std::move;
using std::exception;
using std::atomic;
using std::atomic_fetch_add;
using std::experimental::filesystem::path;
using std::experimental::filesystem::absolute;


void chunk_sequences_into_regions(vector<Region>& regions, unordered_map<string,SequenceElement>& sequences, uint64_t chunk_size){
    ///
    /// Take all the sequences in some iterable object and chunk their lengths
    ///

    // For every sequence
    for (auto& [name, item]: sequences){
        chunk_sequence(regions, name, chunk_size, item.sequence.size());
    }
}


void load_fasta_sequences_from_file(path& fasta_path,
        vector <pair <string, FastaIndex> >& read_index_vector,
        unordered_map<string, SequenceElement>& sequences,
        mutex& map_mutex,
        atomic<uint64_t>& job_index){

    while (job_index < read_index_vector.size()) {
        // Fetch add
        uint64_t thread_job_index = job_index.fetch_add(1);

        // Initialize containers
        SequenceElement sequence;

        // Fetch Fasta sequence
        FastaReader fasta_reader = FastaReader(fasta_path);

        // Get read name and index
        string read_name = read_index_vector[thread_job_index].first;
        uint64_t read_index = read_index_vector[thread_job_index].second.byte_index;

        // First of each element is read_name, second is its index
        fasta_reader.get_sequence(sequence, read_name, read_index);

        // Append the sequence to a map of names:sequence
        map_mutex.lock();
        sequences[sequence.name] = move(sequence);
        map_mutex.unlock();

        // Print status update to stdout
        cerr << "\33[2K\rParsed: " << read_name << flush;
    }
}


void load_fasta_file(path input_file_path,
        unordered_map <string, SequenceElement>& sequences,
        uint16_t max_threads) {

    // This reader is used to fetch an index
    FastaReader fasta_reader(input_file_path);

    // Get index
    auto read_indexes = fasta_reader.get_index();

    // Convert the map object into an indexable object
    vector <pair <string, FastaIndex> > read_index_vector;
    get_vector_from_index_map(read_index_vector, read_indexes);

    mutex map_mutex;

    atomic<uint64_t> job_index = 0;
    vector<thread> threads;

    // Launch threads
    for (uint64_t i=0; i<max_threads; i++){
        // Get data to send to threads (must not be sent by reference, or will lead to concurrency issues)
        try {
            // Call thread safe function to RL encode and write to file
            threads.emplace_back(thread(load_fasta_sequences_from_file,
                                        ref(input_file_path),
                                        ref(read_index_vector),
                                        ref(sequences),
                                        ref(map_mutex),
                                        ref(job_index)));
        } catch (const exception &e) {
            cerr << e.what() << "\n";
            exit(1);
        }
    }

    // Wait for threads to finish
    for (auto& t: threads){
        t.join();
    }

    cerr << "\n" << flush;
}


void parse_alignments(path bam_path,
                      unordered_map <string,SequenceElement>& ref_sequences,
                      CigarStats& cigar_stats,
                      vector <Region>& regions,
                      atomic <uint64_t>& job_index){
    ///
    ///
    ///

    // Initialize FastaReader and relevant containers
    SequenceElement sequence;

    // Initialize BAM reader and relevant containers
    BamReader bam_reader = BamReader(bam_path);
    AlignedSegment aligned_segment;
    Coordinate coordinate;
    Region region;
    Cigar cigar;

    string ref_name;

    bool filter_secondary = true;
    bool filter_supplementary = false;
    uint16_t map_quality_cutoff = 5;

    // Volatiles
    bool in_left_bound;
    bool in_right_bound;
    string true_base;
    string observed_base;

    uint8_t match_code = Cigar::cigar_code_key.at("=");
    uint8_t mismatch_code = Cigar::cigar_code_key.at("X");
    uint8_t insert_code = Cigar::cigar_code_key.at("I");
    uint8_t delete_code = Cigar::cigar_code_key.at("D");

    // Only allow matches
    unordered_set<uint8_t> valid_cigar_codes = {match_code,
                                                mismatch_code,
                                                insert_code,
                                                delete_code};

    while (job_index < regions.size()) {
        uint64_t thread_job_index = job_index.fetch_add(1);
        region = regions.at(thread_job_index);

        // BAM coords are 1 based
        bam_reader.initialize_region(region.name, region.start+1, region.stop+1);

        int i = 0;

        while (bam_reader.next_alignment(aligned_segment, map_quality_cutoff, filter_secondary, filter_supplementary)) {
            // Iterate cigars that match the criteria (must be '=')
            while (aligned_segment.next_valid_cigar(coordinate, cigar, valid_cigar_codes)) {
                aligned_segment.update_containers(coordinate, cigar);
                aligned_segment.increment_coordinate(coordinate, cigar);

                in_left_bound = (int64_t(region.start) <= coordinate.ref_index);
                in_right_bound = (coordinate.ref_index - 1 < int64_t(region.stop));

                // Subset alignment to portions of the read that are within the window/region
                if (in_left_bound and in_right_bound) {

                    // Count up the operations
                    if (cigar.code == match_code){
                        cigar_stats.n_matches += cigar.length;
                    }
                    else if (cigar.code == mismatch_code){
                        cigar_stats.n_mismatches += cigar.length;
                    }
                    else if (cigar.code == delete_code){
                        if (cigar.length < 50){     //TODO un-hardcode this <--
                            cigar_stats.n_deletes += cigar.length;
                        }
                        else {
                            cout << region.name << '\t' << coordinate.ref_index << "\tD\t" << cigar.length << '\n';
                        }
                    }
                    else if (cigar.code == insert_code){
                        if (cigar.length < 50){     //TODO un-hardcode this <--
                            cigar_stats.n_inserts += cigar.length;
                        }
                        else {
                            cout << region.name << '\t' << coordinate.ref_index << "\tI\t" << cigar.length << '\n';
                        }
                    }
                    else{
                        throw runtime_error("ERROR: unexpected cigar operation: \n" + cigar.to_string());
                    }

                    cigar_stats.cigar_lengths[cigar.code][cigar.length]++;
                }
                else{
                    // Out of bounds
                }

                coordinate = {};
                cigar = {};
            }

            i++;
        }

        cerr << "\33[2K\rParsed: " << region.to_string() << flush;
    }
}


CigarStats get_fasta_cigar_stats(path bam_path,
                                       unordered_map <string,SequenceElement>& ref_sequences,
                                       vector <Region>& regions,
                                       uint16_t max_threads){
    ///
    ///
    ///
    vector<CigarStats> stats_per_thread(max_threads);

    vector<thread> threads;
    atomic<uint64_t> job_index = 0;

    // Launch threads
    for (uint64_t i=0; i<max_threads; i++){
        try {
            // Call thread safe function to read and write to file
            threads.emplace_back(thread(parse_alignments,
                                        ref(bam_path),
                                        ref(ref_sequences),
                                        ref(stats_per_thread[i]),
                                        ref(regions),
                                        ref(job_index)));
        } catch (const exception &e) {
            cerr << e.what() << "\n";
            exit(1);
        }
    }

    // Wait for threads to finish
    for (auto& t: threads){
        t.join();
    }
    cerr << "\n" << flush;

    cerr << "Summing cigar operations from " << max_threads << " threads...\n";

    CigarStats cigar_stats_sum;

    for (auto& element: stats_per_thread){
        cigar_stats_sum += element;
    }

    return cigar_stats_sum;
}


void measure_identity_from_fasta(path reads_fasta_path,
        path reference_fasta_path,
        path output_directory,
        uint16_t max_threads){

    cerr << "Using " + to_string(max_threads) + " threads\n";

    // How big (bp) should the regions be for iterating the BAM? Regardless of size,
    // only one alignment worth of RAM is consumed per chunk. This value should be chosen as an appropriate
    // fraction of the genome size to prevent threads from being starved. Larger chunks also reduce overhead
    // associated with iterating reads that extend beyond the region (at the edges)
    uint64_t chunk_size = 100;

    // Initialize readers
    FastaReader reads_fasta_reader = FastaReader(reads_fasta_path);
    FastaReader ref_fasta_reader = FastaReader(reference_fasta_path);

    // Store ref sequences in memory
    unordered_map<string,SequenceElement> ref_sequences;
    load_fasta_file(reference_fasta_path, ref_sequences, max_threads);

    // Index all the files in the MP directory
    // Setup Alignment parameters
    bool sort = true;
    bool index = true;
    bool delete_intermediates = false;  //TODO: switch to true
    uint16_t k = 15;
    string minimap_preset = "asm20";    //TODO: make command line argument?
    bool explicit_mismatch = true;

    // Align reads to the reference
    path bam_path;
    bam_path = align(reference_fasta_path,
            reads_fasta_path,
            output_directory,
            sort,
            index,
            delete_intermediates,
            k,
            minimap_preset,
            explicit_mismatch,
            max_threads);

    // Chunk alignment regions
    vector<Region> regions;
    chunk_sequences_into_regions(regions, ref_sequences, chunk_size);

    cerr << "Iterating alignments...\n" << std::flush;

    reads_fasta_reader.index();
    unordered_map<string,FastaIndex> read_indexes = reads_fasta_reader.get_index();

    // Launch threads for parsing alignments and generating matrices
    CigarStats stats = get_fasta_cigar_stats(bam_path,
            ref_sequences,
            regions,
            max_threads);

    cerr << '\n';

    cout << "identity (M/(M+X+I+D)):\t" << stats.calculate_identity() << '\n';
    cout << stats.to_string();
}


void measure_identity_from_bam(path bam_path,
        path reference_fasta_path,
        uint16_t max_threads){

    cerr << "Using " + to_string(max_threads) + " threads\n";

    // How big (bp) should the regions be for iterating the BAM? Regardless of size,
    // only one alignment worth of RAM is consumed per chunk. This value should be chosen as an appropriate
    // fraction of the genome size to prevent threads from being starved. Larger chunks also reduce overhead
    // associated with iterating reads that extend beyond the region (at the edges)
    uint64_t chunk_size = 10*1000*1000;

    // Initialize readers
    FastaReader ref_fasta_reader = FastaReader(reference_fasta_path);

    // Store ref sequences in memory
    unordered_map<string,SequenceElement> ref_sequences;
    load_fasta_file(reference_fasta_path, ref_sequences, max_threads);

    // Chunk alignment regions
    vector<Region> regions;
    chunk_sequences_into_regions(regions, ref_sequences, chunk_size);

    cerr << "Iterating alignments...\n" << std::flush;

    // Launch threads for parsing alignments and generating matrices
    CigarStats stats = get_fasta_cigar_stats(bam_path,
            ref_sequences,
            regions,
            max_threads);

    cerr << '\n';

    cout << "identity (M/(M+X+I+D)):\t" << stats.calculate_identity() << '\n';
    cout << stats.to_string();
}