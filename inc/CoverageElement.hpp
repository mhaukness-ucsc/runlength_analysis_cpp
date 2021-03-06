
#ifndef RUNLENGTH_ANALYSIS_COVERAGEELEMENT_HPP
#define RUNLENGTH_ANALYSIS_COVERAGEELEMENT_HPP

#include "Base.hpp"
#include <string>
#include <vector>
#include <iostream>


class CoverageElement{
public:
    /// Attributes ///

    float weight;
    uint16_t length;
    char base;
    bool reversal;

    CoverageElement(char base, uint16_t length, bool reversal, float weight);
    static const string reversal_string;
    static const string reversal_string_plus_minus;

    /// Methods ///
    string to_string();
    uint8_t get_base_index();
    bool is_conventional_base();
};

#endif //RUNLENGTH_ANALYSIS_COVERAGEELEMENT_HPP
