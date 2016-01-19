/*
 * SplitSequence.cpp
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#include <SplitSequence.h>

namespace RiceAlgorithm
{

//SplitSequence::SplitSequence(size_t sampleBlockSize, CodingSelection selection)
SplitSequence::SplitSequence(size_t sampleBlockSize)
: AdaptiveEntropyEncoder(sampleBlockSize)
{
	// TODO Auto-generated constructor stub

}

SplitSequence::~SplitSequence()
{
	// TODO Auto-generated destructor stub
}

unsigned int SplitSequence::encode(unsigned int* encodedBlock)
{

    unsigned int code_len = (unsigned int)-1;
    int i = 0, k = 0;
    int k_limit = 14;

    for(k = 0; k < k_limit; k++)
    {
        unsigned int code_len_temp = 0;
        for(i = 0; i < BlockSize; i++)
        {
            code_len_temp += (myInputSamples[i] >> k) + 1 + k;
        }

        if(code_len_temp < code_len)
        {
            code_len = code_len_temp;
            //*k_split = k;
        }
    }

    return code_len;
}

} /* namespace RiceAlgorithm */
