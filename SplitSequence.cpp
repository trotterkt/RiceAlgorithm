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

unsigned int SplitSequence::encode(unsigned int* encodedBlock, CodingSelection &selection)
{

    unsigned int code_len = (unsigned int)-1;
    int i = 0, k = 0;
    int k_limit = 14;


    for(k = 0; k < k_limit; k++)
    {

        unsigned int code_len_temp = 0;
        for(i = 0; i < 32; i++)
        {
        	cout << "myInputSamples[i] >> k = " << (myInputSamples[i] >> k) << ", k=" << k << endl;

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

    boost::dynamic_bitset<> encodedStream(totalEncodedSize, 1ul);

    // assign each encoded sample and shift by the next one
    // at the end of the loop, we will assign the last one
    for(int index = 1; index < 32; index++)
    {
        encodedStream[0] |= 1;
        encodedStream <<= encodedSizeList[index];        
    }
    encodedStream[0] |= 1;
    cout << "encodedStream(size:" << encodedStream.size() << ")= " << encodedStream << endl;

    size_t numberEncodedBytes;
    
    if(totalEncodedSize % 2)
    {
        numberEncodedBytes = totalEncodedSize/8 + 1; // capture partial byte
    }
    else
    {
        numberEncodedBytes = totalEncodedSize/8; 
    }
    
    
    myEncodedBlockSize = code_len;
    return code_len;
}

} /* namespace RiceAlgorithm */
