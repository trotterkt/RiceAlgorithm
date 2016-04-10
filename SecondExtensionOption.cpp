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

    unsigned int code_len = 0;
    unsigned int secondExtentionOption;


    int i = 0;
    for(i = 0; i < 32; i+=2)
    {
        secondExtentionOption = (((unsigned int)myInputSamples[i] + myInputSamples[i + 1])*((unsigned int)myInputSamples[i] + myInputSamples[i + 1] + 1))/2 + myInputSamples[i + 1];
        code_len += secondExtentionOption + 1;

        // :TODO: This encoding does not seem to be applicable to my current test image.
        // When it does may need to reassess how encoded samples are read back out before
        // sending encodedStream
        encodedStream.append(secondExtentionOption);
    }

    return code_len;
}

void SecondExtensionOption::decode(CodingSelection selection, ushort* splitValue, ushort* encodedStream, ushort* preprocessedStream)
{
	selection = SecondExtensionOpt;

    unsigned int code_len = 0;
    unsigned int secondExtentionOption;


    int i = 0;
    for(i = 0; i < 32; i+=2)
    {
        secondExtentionOption = (((unsigned int)myInputSamples[i] + myInputSamples[i + 1])*((unsigned int)myInputSamples[i] + myInputSamples[i + 1] + 1))/2 + myInputSamples[i + 1];
        code_len += secondExtentionOption + 1;

        // :TODO: This encoding does not seem to be applicable to my current test image.
        // When it does may need to reassess how encoded samples are read back out before
        // sending encodedStream
//        encodedStream.append(secondExtentionOption);
    }

}

} /* namespace RiceAlgorithm */
