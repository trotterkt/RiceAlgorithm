/*
 * AdaptiveEntropyEncoder.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
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
const ushort BitsPerByte(8u);

enum CodingSelection {K0=1,             // Fundamental Sequence (FS) is the same as Split-Sequence, with K=0
					  K1, K2, K3, K4, K5,
					  K6, K7, K8, K9, K10,
					  K11, K12, K13,
					  ZeroBlockOpt=0, SecondExtensionOpt=-1, NoCompressionOpt=15};

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
    void setSamples(ushort* samples) { myInputSamples = samples; }

    // encoding in the base class is basically nothing, and this is also the same as no compression option
    virtual unsigned int encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection)
	                                                                                   {
        																					size_t totalEncodedSize(0);

																							memcpy(myEncodedBlock, myInputSamples, BlockSize * sizeof(ushort));
																						    myCodingSelection = NoCompressionOpt;
																							selection = NoCompressionOpt; // :TODO: is this necessary any more?

																							totalEncodedSize = 32 * sizeof(ushort);
        																					// include space for the  code option
        																				    //totalEncodedSize += CodeOptionBitFieldFundamentalOrNoComp;

																							//encodedStream.append(int(selection));

    																				        encodedStream.resize((totalEncodedSize * BitsPerByte) + CodeOptionBitFieldFundamentalOrNoComp);

       																				        // see Lossless Data Compression, Blue Book, sec 5.1.2
        																				    // place the code encoding selection
        																				    boost::dynamic_bitset<> encodedSelectionStream((totalEncodedSize*BitsPerByte)+CodeOptionBitFieldFundamentalOrNoComp, selection);
        																			        //encodedStream <<= CodeOptionBitFieldFundamentalOrNoComp;
        																			        encodedStream |= encodedSelectionStream;
        																			        //encodedStream <<= (sizeof(ushort) * BitsPerByte);


        																				    for(int index = 0; index < 32; index++)
        																				    {
        																				        ushort sample = myInputSamples[index];

        																				        //:TODO: this section appears to be responsible for about 8 seconds in the
        																				        // total encoding time
        																				        //===================================================================================
        															        				    encodedStream <<= (sizeof(ushort) * BitsPerByte);
        																				        boost::dynamic_bitset<> encodedSample((encodedStream.size()), sample);
        																				        encodedStream |= encodedSample;
        																				        //===================================================================================
        																				    }



        																				    return (encodedStream.size() - CodeOptionBitFieldFundamentalOrNoComp);
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
