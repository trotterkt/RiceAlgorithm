/*
 * AdaptiveEntropyEncoder.h
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#ifndef ADAPTIVEENTROPYENCODER_H_
#define ADAPTIVEENTROPYENCODER_H_

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <Predictor.h>
#include <boost/dynamic_bitset.hpp>


namespace RiceAlgorithm
{
const size_t BlockSize(32*DynamicRange);
const ushort BitsPerByte(8);

enum CodingSelection {K0=0,             // Fundamental Sequence (FS) is the same as Split-Sequence, with K=0
					  K1, K2, K3, K4, K5,
					  K6, K7, K8, K9, K10,
					  K11, K12, K13, K14,
					  SplitSeq, ZeroBlockOpt, SecondExtensionOpt, NoCompressionOpt};

//:TODO: will probably change this to be more in line with a Factory pattern


const short CodeOptionBitFieldZeroOrSecEx(5);
const short CodeOptionBitFieldFundamentalOrNoComp(4);


class AdaptiveEntropyEncoder
{

public:
	AdaptiveEntropyEncoder(size_t sampleBlockSize);
	virtual ~AdaptiveEntropyEncoder();
	AdaptiveEntropyEncoder(const AdaptiveEntropyEncoder &right);

	//:TODO: this should probably be in constructor
	void setSamples(ushort* samples) { memcpy(myInputSamples, samples, myEncodedBlockSize * sizeof(ushort)); }

    // encoding in the base class is basically nothing, and this is also the same as no compression option
	virtual unsigned int encode(unsigned int* encodedBlock, boost::dynamic_bitset<> &encodedStream, CodingSelection &selection) {
																							memcpy(myEncodedBlock, myInputSamples, BlockSize * sizeof(ushort));
																							memcpy(encodedBlock, myInputSamples, BlockSize * sizeof(ushort)); // :TODO: is this necessary any more?
																						    myCodingSelection = NoCompressionOpt;
																							selection = NoCompressionOpt; // :TODO: is this necessary any more?

																						    return BlockSize;
																						};

	CodingSelection getSelection() { return myCodingSelection; }
	ushort* getEncodedBlock() { return myEncodedBlock; }
	size_t getEncodedBlockSize() { return myEncodedBlockSize; }

	 bool operator<(unsigned int encodedSize){
		 	 	 	 	 	 	 	 	 	  	  if(myEncodedBlockSize < encodedSize)
		 	 	 	 	 	 	 	 	 	  	  {
		 	 	 	 	 	 	 	 	 	 	 	 return true;
		 	 	 	 	 	 	 	 	 	  	  }

		 	 	 	 	 	 	 	 	 	  	  return false;
	 	 	 	 	 	 	 	 	 	 	   }

protected:
	ushort* myInputSamples; // use input for final encoding as well, with sync frame
	size_t myBlockSize;
    CodingSelection myCodingSelection;
    ushort* myEncodedBlock;
    size_t myEncodedBlockSize;

    // append all encoded blocks before sending at once
    boost::dynamic_bitset<> myFullEncodedStream;

    inline void appendLsb(boost::dynamic_bitset<> &data, boost::dynamic_bitset<> &appendData)
    {
    	size_t sizeAppend = appendData.size();

    	data.resize(data.size() + sizeAppend);
    	data <<= sizeAppend;
    	data |= appendData;
    }


};

} /* namespace RiceAlgorithm */

#endif /* ADAPTIVEENTROPYENCODER_H_ */
