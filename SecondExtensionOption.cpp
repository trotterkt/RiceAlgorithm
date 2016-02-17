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
	selection = SecondExtensionOpt;

	encodedStream.resize(32);

    unsigned int code_len = 0;
    int i = 0;
    for(i = 0; i < 32; i+=2)
    {
    	encodedStream[i/2] = (((unsigned int)myInputSamples[i] + myInputSamples[i + 1])*((unsigned int)myInputSamples[i] + myInputSamples[i + 1] + 1))/2 + myInputSamples[i + 1];
        code_len += encodedStream[i/2] + 1;
    }

    //:TODO: This encoding is incorrect - fix
    // return code_len;
    return BlockSize;

}

} /* namespace RiceAlgorithm */
