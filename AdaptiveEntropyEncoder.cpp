/*
 * AdaptiveEntropyEncoder.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#include <AdaptiveEntropyEncoder.h>


namespace RiceAlgorithm
{


AdaptiveEntropyEncoder::AdaptiveEntropyEncoder(size_t sampleBlockSize)
	: myBlockSize(sampleBlockSize), myEncodedBlockSize(BlockSize)
{

    myInputSamples = new ushort[sampleBlockSize];
    myEncodedBlock = new ushort[myEncodedBlockSize];

}

AdaptiveEntropyEncoder::~AdaptiveEntropyEncoder()
{
	delete [] myInputSamples;
	delete [] myEncodedBlock;
}

AdaptiveEntropyEncoder::AdaptiveEntropyEncoder(const AdaptiveEntropyEncoder &right)
{
    myInputSamples = new ushort[right.myBlockSize];
	memcpy(myInputSamples, right.myInputSamples, right.myBlockSize*sizeof(ushort));
    myBlockSize = right.myBlockSize;

    myEncodedBlockSize = right.myEncodedBlockSize;
    myEncodedBlock = new ushort[myEncodedBlockSize];
	memcpy(myEncodedBlock, right.myEncodedBlock, myEncodedBlockSize);
}


} /* namespace RiceAlgorithm */
