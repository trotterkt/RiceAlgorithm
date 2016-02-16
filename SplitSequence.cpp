/*
 * SplitSequence.cpp
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#include <SplitSequence.h>
#include <iostream>
#include <vector>
#include <boost/dynamic_bitset.hpp>

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

unsigned int SplitSequence::encode(unsigned int* encodedBlock, boost::dynamic_bitset<> &encodedStream, CodingSelection &selection)
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
            selection = RiceAlgorithm::CodingSelection(k);
        }
    }


    vector< size_t > encodedSizeList;
    size_t totalEncodedSize(0);

    // Get the total encoded size first
    for(int index = 0; index < 32; index++)
    {
        size_t encodedSize = (myInputSamples[index] >> selection) + 1;
        encodedSizeList.push_back(encodedSize);
        totalEncodedSize += encodedSize;
    }

    // include space for the  code option
    totalEncodedSize += CodeOptionBitFieldFundamentalOrNoComp;

    //boost::dynamic_bitset<> encodedStream(totalEncodedSize, 1ul);
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
    boost::dynamic_bitset<> maskBits(selection, 0xffff);
    ulong mask = maskBits.to_ulong();

    for(int index = 0; index < 32; index++)
    {
        ushort  maskedSample = myInputSamples[index] & mask;
        boost::dynamic_bitset<> encodedSample(selection, maskedSample);
        
        encodedStream.resize(totalEncodedSize + selection);
        encodedStream <<= selection;
        encodedSample.resize(totalEncodedSize + selection);
        encodedStream |= encodedSample;
        
        totalEncodedSize = encodedStream.size();
    }

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
    boost::dynamic_bitset<> encodedSelectionStream(totalEncodedSize, (selection + 1));
    encodedSelectionStream <<= (totalEncodedSize-CodeOptionBitFieldFundamentalOrNoComp);
    encodedStream |= encodedSelectionStream;

    // Determine the shift amount if we are not on a full byte, as is probably normally
    // the case. There would otherwise often be leading zero bits
    size_t shiftBits = numberEncodedBytes*BitsPerByte - encodedStream.size();
    myEncodedBlockSize = encodedStream.size();

    encodedStream.resize(encodedStream.size() + shiftBits);
    encodedStream <<= shiftBits;

     //cout << "totalEncodedSize=" << totalEncodedSize << ", encodedStream(size:" << encodedStream.size() << ")= " << encodedStream << endl;

    return code_len;
}

} /* namespace RiceAlgorithm */
