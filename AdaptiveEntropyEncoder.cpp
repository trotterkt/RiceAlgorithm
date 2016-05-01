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
#include <Endian.h>
#include <iostream>
#include <ShiftFunctions.h>

using namespace std;

namespace RiceAlgorithm
{


AdaptiveEntropyEncoder::AdaptiveEntropyEncoder(size_t sampleBlockSize)
	: myBlockSize(sampleBlockSize), myEncodedBlockSize(BlockSize)
{
    myEncodedBlock = new ushort[myEncodedBlockSize];
}

AdaptiveEntropyEncoder::~AdaptiveEntropyEncoder()
{
	delete [] myEncodedBlock;
}

AdaptiveEntropyEncoder::AdaptiveEntropyEncoder(const AdaptiveEntropyEncoder &right)
{
    myInputSamples = right.myInputSamples;
    myBlockSize = right.myBlockSize;

    myEncodedBlockSize = right.myEncodedBlockSize;
    myEncodedBlock = new ushort[myEncodedBlockSize];
	memcpy(myEncodedBlock, right.myEncodedBlock, myEncodedBlockSize);
}

void AdaptiveEntropyEncoder::decode(CodingSelection selection, ushort* splitValue, unsigned char* encodedStream, ulong blockIndex, ushort* preprocessedStream)
{
	// counting all of the values in the splitValue array of 32 elements
	// provides the location of the beginning bits for the split values
	// in bits
	int bitLocation(0);
    for(int index=0; index < 32; index++)
    {
    	bitLocation += splitValue[index];
    }

    // Make a new array for the 32 split values
	const unsigned int CopySize(32 * sizeof(ushort) + 1); // Encoded data will be no larger than this
	unsigned char encodedDataCopy[CopySize];
	memcpy(encodedDataCopy, encodedStream, CopySize);


    size_t bufferSize(CopySize * BitsPerByte);

	for(int index=0; index<32; index++)
	{
		ushort value(0);

		memcpy(&value, &reinterpret_cast<ushort*>(&encodedDataCopy)[index], sizeof(ushort));

        bigEndianVersusLittleEndian(value);

		preprocessedStream[index + blockIndex*32] =  value;
	}

}


} /* namespace RiceAlgorithm */
