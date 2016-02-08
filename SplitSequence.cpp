/*
 * SplitSequence.cpp
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#include <SplitSequence.h>
#include <iostream>
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

//            //:TODO: Not working as expected.
//            //===========================================================================
//            boost::dynamic_bitset<> nextSample(((myInputSamples[i] >> k) + 1), 1ul);
//
//            // don't bother if the current size is >= BlockSize
//            if(code_len_temp < BlockSize)
//            {
//				//:TODO: do I need header info before?
//				append(nextSample, completeBitStream);
//				completeBitStream = nextSample;
//				cout << completeBitStream << endl;
//            }
//            //===========================================================================

        }


        if(code_len_temp < code_len)
        {
            code_len = code_len_temp;
            selection = RiceAlgorithm::CodingSelection(k);

//            //=======================================================
//
//            ulong value = completeBitStream.to_ulong();
//            cout << "size=" << completeBitStream.size() << "  value=" << value << " bits=" << completeBitStream << endl;
//            //=======================================================
        }
    }

    // Just calculate the encoded sample for the winner
    // this is used to assemble the encoded block, one sample at a time
    boost::dynamic_bitset<> completeBitStream(((myInputSamples[31] >> selection)+1), 1ul);

    cout << completeBitStream << endl;

    for(i = 30; i >= 0; i--)
    {
    	// determine the new size
    	size_t newSize = completeBitStream.size() + ((myInputSamples[i] >> selection)+1);
    	completeBitStream.resize(newSize);

    	completeBitStream.set(completeBitStream.size()-1, true);

    	completeBitStream >> ((myInputSamples[i] >> selection) + 1);
    	//completeBitStream[0] |= 1;

        cout << completeBitStream << endl;
    }


    myEncodedBlockSize = code_len;
    return code_len;
}

} /* namespace RiceAlgorithm */
