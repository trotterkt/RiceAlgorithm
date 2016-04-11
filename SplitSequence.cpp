/*
 * SplitSequence.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#include <SplitSequence.h>
#include <iostream>
#include <ShiftFunctions.h>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <Timing.h>
#include <Endian.h>

using namespace std;

/* Append the lower-order nbits bits from value to set. */
template <typename T>
void append(boost::dynamic_bitset<> &set, T value, size_t nbits)
{
    set.resize(set.size() + nbits);
    for (size_t i=1; i<=nbits; i++)
    {
        set[set.size() - i] = value & 1;
        value >>= 1;
    }


}


void append(boost::dynamic_bitset<> &setLSB, boost::dynamic_bitset<> &setMSB)
{
    size_t newSize = setLSB.size() + setMSB.size();
	setLSB.resize(newSize);
	setMSB.resize(newSize);

	setMSB <<= setLSB.size();

	setMSB |= setLSB;
}


namespace RiceAlgorithm
{

SplitSequence::SplitSequence(size_t sampleBlockSize)
: AdaptiveEntropyEncoder(sampleBlockSize)
{
	// TODO Auto-generated constructor stub

}

SplitSequence::~SplitSequence()
{
	// TODO Auto-generated destructor stub
}

unsigned int SplitSequence::encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection)
{
    unsigned int code_len = (unsigned int)-1;
    int i = 0, k = 0;
    int k_limit = 14;


    for(k = 0; k < k_limit; k++)
    {

        unsigned int code_len_temp = 0;
        for(i = 0; i < 32; i++)
        {
        	ushort encodedSample = myInputSamples[i] >> k;
            code_len_temp += (encodedSample) + 1 + k;
        }

        if(code_len_temp < code_len)
        {
            code_len = code_len_temp;
            selection = RiceAlgorithm::CodingSelection(k+1);
        }
    }


    size_t encodedSizeList[32];
    size_t totalEncodedSize(0);

    // Get the total encoded size first
    for(int index = 0; index < 32; index++)
    {
        size_t encodedSize = (myInputSamples[index] >> (selection-1)) + 1;
        encodedSizeList[index] = encodedSize;
        totalEncodedSize += encodedSize;
    }


    // include space for the  code option
    totalEncodedSize += CodeOptionBitFieldFundamentalOrNoComp;

    encodedStream.resize(totalEncodedSize);
    encodedStream.reset();
    encodedStream[0] = 1;

    // assign each encoded sample and shift by the next one
    // at the end of the loop, we will assign the last one
    for(int index = 1; index < 32; index++)
    {
        encodedStream[0] |= 1;
        encodedStream <<= encodedSizeList[index];        
    }
    encodedStream[0] |= 1;

    //cout << "totalEncodedSize=" << totalEncodedSize << ", encodedStream(size:" << encodedStream.size() << ")= " << encodedStream << endl;

    // after the zero sequence number that was split off, then we add that value to the stream
    // for each of the samples
    boost::dynamic_bitset<> maskBits((selection-1), 0xffff);
    ulong mask = maskBits.to_ulong();

    for(int index = 0; index < 32; index++)
    {
        ushort maskedSample = myInputSamples[index] & mask;

        //:TODO: this section appears to be responsible for about 8 seconds in the
        // total encoding time
        //===================================================================================
        boost::dynamic_bitset<> encodedSample((selection-1), maskedSample);

        encodedStream.resize(totalEncodedSize + (selection-1));
        encodedStream <<= (selection-1);
        encodedSample.resize(totalEncodedSize + (selection-1));
        encodedStream |= encodedSample;

        totalEncodedSize = encodedStream.size();
        //===================================================================================
    }
    //cout << "totalEncodedSize=" << totalEncodedSize << ", encodedStream(size:" << encodedStream.size() << ")= " << encodedStream << endl;

//    // Alternate approach
//    //************************************************
//    // what is the largest size in bits possible? 32*K14 = 448 bits = 56 bytes
//    unsigned long long encodedSample2(0); // 8 bytes == 64 bits
//    size_t additionalEncodedSize = selection * 32;
//    totalEncodedSize = encodedStream.size() + additionalEncodedSize;
//    encodedStream.resize(totalEncodedSize);
//    encodedStream <<= additionalEncodedSize;
//
//
//    for(int index = 0; index < 32; index++)
//    {
//        ushort maskedSample = myInputSamples[index] & mask;
//        encodedSample2 |= maskedSample;
//        encodedSample2 <<= selection;
//    }
//    boost::dynamic_bitset<> mergedEncodedSample(totalEncodedSize, encodedSample2);
//    encodedStream |= mergedEncodedSample;
//    //************************************************


    size_t numberEncodedBytes;
    
    if(totalEncodedSize % BitsPerByte)
    {
        numberEncodedBytes = totalEncodedSize/BitsPerByte + 1; // capture partial byte
    }
    else
    {
        numberEncodedBytes = totalEncodedSize/BitsPerByte; 
    }
    
	// see Lossless Data Compression, Blue Book, sec 5.1.2
    // place the code encoding selection
    boost::dynamic_bitset<> encodedSelectionStream(totalEncodedSize, selection);
    encodedSelectionStream <<= (totalEncodedSize-CodeOptionBitFieldFundamentalOrNoComp);
    encodedStream |= encodedSelectionStream;

    //cout << "totalEncodedSize=" << totalEncodedSize << ", encodedStream(size:" << encodedStream.size() << ")= " << encodedStream << endl;

    // Determine the shift amount if we are not on a full byte, as is probably normally
    // the case. There would otherwise often be leading zero bits
    size_t shiftBits = numberEncodedBytes*BitsPerByte - encodedStream.size();
    myEncodedBlockSize = encodedStream.size();

    encodedStream.resize(encodedStream.size() + shiftBits);
    encodedStream <<= shiftBits;

    // cout << "totalEncodedSize=" << totalEncodedSize << ", encodedStream(size:" << encodedStream.size() << ")= " << encodedStream << endl;

    return code_len;
}

void SplitSequence::decode(CodingSelection selection, ushort* splitValue, unsigned char* encodedStream, ulong blockIndex, ushort* preprocessedStream)
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
	const unsigned int CopySize(32 * sizeof(ushort)); // Encoded data will be no larger than this
	unsigned char encodedDataCopy[CopySize];
	memcpy(encodedDataCopy, encodedStream, CopySize);


    // Combine the individual values per the split sequence method
    // and save in the preprocessed array
	size_t bufferSize(CopySize * BitsPerByte);

	shiftLeft(encodedDataCopy, bufferSize, bitLocation);

	for(int index=0; index<32; index++)
	{
		ushort value(0);
		memcpy(&value, encodedDataCopy, sizeof(ushort));
		bigEndianVersusLittleEndian(value);

		value >>= (sizeof(ushort) * BitsPerByte - (selection - 1));
		preprocessedStream[index + blockIndex*32] = ((splitValue[index]-1) << (selection - 1)) |  value;

		shiftLeft(encodedDataCopy, bufferSize, (selection - 1));
	}

}

} /* namespace RiceAlgorithm */
