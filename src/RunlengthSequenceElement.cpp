
#include "RunlengthSequenceElement.hpp"
#include "Base.hpp"


//SequenceElement::SequenceElement()=default;


void RunlengthSequenceElement::get_ref_data(vector<float>& ref_data, int64_t index){
    ref_data = {};

//    cout << "\nREF:\n" << this->sequence << '\n';
//    cout << "\nINDEX:\n" << index << '\n';
//    cout << "\nLENGTH:\n" << this->sequence.size() << '\n';
//    cout << "\nBASE:\n" << string(1,this->sequence[index]) << '\n';

    float base = base_to_float(this->sequence[index]);
    ref_data.emplace_back(base);
    ref_data.emplace_back(float(0));
    ref_data.emplace_back(float(this->lengths[index]));
}


void RunlengthSequenceElement::get_read_data(vector<float>& read_data, Cigar& cigar, Coordinate& coordinate, AlignedSegment& alignment){
    read_data = {};

    if (cigar.is_read_move()) {
        float base = base_to_float(this->sequence[coordinate.read_true_index]);

        // Complement base if necessary
        if (alignment.reversal and is_valid_base_index(base)) {
            base = 3 - base;
        }

        read_data.emplace_back(base);
        read_data.emplace_back(float(alignment.reversal));
        read_data.emplace_back(float(this->lengths[coordinate.read_true_index]));
    }
    else{
        read_data.emplace_back(Pileup::DELETE_CODE);
        read_data.emplace_back(float(alignment.reversal));
        read_data.emplace_back(float(0));
    }
}


void RunlengthSequenceElement::generate_default_data_vector(vector<float>& read_data){
    read_data = {};
    read_data.emplace_back(Pileup::EMPTY);
    read_data.emplace_back(float(0));
    read_data.emplace_back(float(0));
}
