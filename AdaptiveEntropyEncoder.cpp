/*
 * AdaptiveEntropyEncoder.cpp
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#include <AdaptiveEntropyEncoder.h>


namespace RiceAlgorithm
{


AdaptiveEntropyEncoder::AdaptiveEntropyEncoder(size_t sampleBlockSize)
	: myBlockSize(sampleBlockSize)
{

    //myInputSamples = new ushort[sampleBlockSize];
    //int* pInt = new int;

}

AdaptiveEntropyEncoder::~AdaptiveEntropyEncoder()
{
	//delete [] myInputSamples;
}

AdaptiveEntropyEncoder::AdaptiveEntropyEncoder(const AdaptiveEntropyEncoder &right)
{
	//memcpy(myInputSamples, right.myInputSamples, myBlockSize*sizeof(ushort));
    //myBlockSize = right.myBlockSize;
}


} /* namespace RiceAlgorithm */
