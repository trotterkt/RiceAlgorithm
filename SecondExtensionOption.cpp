/*
 * SecondExtensionOption.cpp
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
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

unsigned int SecondExtensionOption::encode(unsigned int* encodedBlock)
{
    unsigned int code_len = 0;
    int i = 0;
    for(i = 0; i < 32; i+=2)
    {
    	encodedBlock[i/2] = (((unsigned int)myInputSamples[i] + myInputSamples[i + 1])*((unsigned int)myInputSamples[i] + myInputSamples[i + 1] + 1))/2 + myInputSamples[i + 1];
        code_len += encodedBlock[i/2] + 1;
    }
}

} /* namespace RiceAlgorithm */
