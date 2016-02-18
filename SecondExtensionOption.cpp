/*
 * SecondExtensionOption.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#include <SecondExtensionOption.h>

namespace RiceAlgorithm
{

SecondExtensionOption::SecondExtensionOption(size_t sampleBlockSize)
	: AdaptiveEntropyEncoder(sampleBlockSize)
{
	// TODO Auto-generated constructor stub

}

SecondExtensionOption::~SecondExtensionOption()
{
	// TODO Auto-generated destructor stub
}

unsigned int SecondExtensionOption::encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection)
{
    //:TODO: This really should be reassessed, since I need to return the 2nd extension values
    // if this encoding is selected. Does not appear applicable for my test image
	selection = SecondExtensionOpt;

    unsigned int code_len = 0;
    unsigned int secondExtentionOption[32];

    int i = 0;
    for(i = 0; i < 32; i+=2)
    {
        secondExtentionOption[i/2] = (((unsigned int)myInputSamples[i] + myInputSamples[i + 1])*((unsigned int)myInputSamples[i] + myInputSamples[i + 1] + 1))/2 + myInputSamples[i + 1];
        code_len += secondExtentionOption[i/2] + 1;
    }

    return code_len;
}

} /* namespace RiceAlgorithm */
