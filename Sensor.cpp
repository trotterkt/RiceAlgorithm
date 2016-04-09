/*
 * Sensor.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#include <Sensor.h>
#include <iostream>
#include <bitset>
#include <vector>
#include <math.h>
#include <limits.h>
#include <Timing.h>
#include <Endian.h>


using namespace std;
using namespace RiceAlgorithm;


Sensor::Sensor(ImagePersistence* image, unsigned int x, unsigned int y, unsigned int z) :
        mySource(image), mySamples(0), myXDimension(x), myYDimension(y), myZDimension(z), myPreprocessor(x, y, z), myWinningEncodedLength((unsigned int)-1)
{

    mySamples = mySource->getSampleData(1); //:TODO: only one scan, need to address multiple

    // Create the encoding types
    size_t bufferSize = myXDimension*myYDimension*myZDimension;

    AdaptiveEntropyEncoder* noComp = new AdaptiveEntropyEncoder(bufferSize);
    SecondExtensionOption* secondExt = new SecondExtensionOption(bufferSize);
    ZeroBlockOption* zeroBlock = new ZeroBlockOption(bufferSize);
    SplitSequence* split = new SplitSequence(bufferSize);


    myEncoderList.push_back(noComp);  // No compression must be the first item
    myEncoderList.push_back(secondExt);
    myEncoderList.push_back(zeroBlock);
    myEncoderList.push_back(split);

    // Prepare for the decompression step
    groundPtr = new GroundSystem(image);
}

Sensor::~Sensor()
{
	delete groundPtr;
}

void Sensor::process()
{
	// The goal hear is to form the telemetry of the data

	sendHeader();

	// :TODO: formalize this a little more
	myEncodedBitCount = 19 * BitsPerByte; // Start with header length

	// The first option must be non-compression
	// so that we can set the block to reference to
//	if(myEncoderList[0]->getSelection() != NoCompressionOpt)
//	{
//		exception wrongTypeException; //:TODO: need title
//
//		throw wrongTypeException;
//	}

	//:TODO: Nest this loop in another and iterate over the next residual block
	std::vector<AdaptiveEntropyEncoder*>::iterator winningIteration;

	CodingSelection winningSelection;
	boost::dynamic_bitset<> encodedStream;
	boost::dynamic_bitset<> winningEncodedStream;

	// To combine encoded blocks, current is dependent on the previous.
	// Specifically need the last byte
	boost::dynamic_bitset<> previousEncodedStream;

	timestamp_t t0_intermediate, t1_intermediate, t2_intermediate, t3_intermediate;

	timestamp_t t0 = getTimestamp();

	// Should only need to get the residuals once for a given raw image set
	ushort* residualsPtr = myPreprocessor.getResiduals(mySamples);

	timestamp_t t1 = getTimestamp();

	cout << "Prediction processing time ==> " << fixed << getSecondsDiff(t0, t1) << " seconds"
			<< endl;

	timestamp_t t2 = getTimestamp();

	int blockIndex(0);
	unsigned int encodedLength(0);
	ulong count(0);

	long totalSamples = myXDimension * myYDimension * myZDimension;

	//:TODO: This is one of the 1st places where we will start looking
	// at applying Amdahl's Law!!!

	for (blockIndex = 0; blockIndex < totalSamples; blockIndex += 32)
	{

		size_t encodedSize(0);

		// Reset for each capture of the winning length
		myWinningEncodedLength = (unsigned int) -1;

		t0_intermediate = getTimestamp();

		// Loop through each one of the possible encoders
		for (std::vector<AdaptiveEntropyEncoder*>::iterator iteration = myEncoderList.begin();
				iteration != myEncoderList.end(); ++iteration)
		{

			encodedStream.clear();

			// 1 block at a time
			(*iteration)->setSamples(&residualsPtr[blockIndex]);

			CodingSelection selection; // This will be most applicable for distinguishing FS and K-split

			encodedLength = (*iteration)->encode(encodedStream, selection);

			// This basically determines the winner
			if (encodedLength < myWinningEncodedLength)
			{
				*this = *(*iteration);
				myWinningEncodedLength = encodedLength;
				winningSelection = selection;

				encodedSize = (*iteration)->getEncodedBlockSize();
				//encodedStream.resize(encodedSize);
				winningEncodedStream = encodedStream;
			}

			//******************************
			//if(blockIndex > 32)
			//  break; // debugging
			//******************************

		}

		t1_intermediate = getTimestamp();

		count++;

		switch (winningSelection)
		{
			case RiceAlgorithm::K0:
			case RiceAlgorithm::K1:
			case RiceAlgorithm::K2:
			case RiceAlgorithm::K3:
			case RiceAlgorithm::K4:
			case RiceAlgorithm::K5:
			case RiceAlgorithm::K6:
			case RiceAlgorithm::K7:
			case RiceAlgorithm::K8:
			case RiceAlgorithm::K9:
			case RiceAlgorithm::K10:
			case RiceAlgorithm::K11:
			case RiceAlgorithm::K12:
			case RiceAlgorithm::K13:
				cout << "And the Winner is: K" << int(winningSelection - 1) << " of code length: "
						<< myWinningEncodedLength << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]"
						<< ", count=" << count << endl;
				break;

			case RiceAlgorithm::SecondExtensionOpt:
				cout << "And the Winner is: 2ndEXT of code length: " << myWinningEncodedLength
				     << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]" << ", count=" << count << endl;
				break;

			case RiceAlgorithm::ZeroBlockOpt:
				cout << "And the Winner is: ZEROBLOCK of code length: " << myWinningEncodedLength
				     << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]" << ", count=" << count << endl;
				break;

			case RiceAlgorithm::NoCompressionOpt:
				cout << "And the Winner is: NOCOMP of code length: " << myWinningEncodedLength
				     << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]" << ", count=" << count << endl;

				break;

			default:
				cout << "Unanticipated encoding -- Exiting" << endl;
				exit(-1);

		}

		//*******************************************
		if((count > 9881) && (count < 9886))
		{
			cout << "winning encoding==>" << winningEncodedStream << endl;
			cout << "        encoding==>" << encodedStream << endl;
		}
		//*******************************************

		ushort partialBits = myEncodedBitCount % BitsPerByte;
		unsigned char lastByte(0);

		// When the last byte written is partial, as compared with the
		// total bits written, capture it so that it can be merged with
		// the next piece of encoded data
		if (getLastByte(&lastByte))
		{
			//cout << "Before partial appendage: " << encodedStream << endl;
			unsigned int appendedSize = winningEncodedStream.size() + partialBits;
			winningEncodedStream.resize(appendedSize);

			boost::dynamic_bitset<> lastByteStream(winningEncodedStream.size(), ulong(lastByte));
			//cout << "lastByteStream (Before): " << lastByteStream << endl;
			lastByteStream <<= (winningEncodedStream.size() - BitsPerByte);
			//cout << "lastByteStream (After ): " << lastByteStream << endl;

			winningEncodedStream |= lastByteStream;
			//cout << "After partial appendage : " << encodedStream << endl;
		}

		myEncodedBitCount += (myWinningEncodedLength + CodeOptionBitFieldFundamentalOrNoComp);

		t2_intermediate = getTimestamp();

		static unsigned int lastWinningEncodedLength(0);

		ulong byteCount(0);
		int additionalBits(myEncodedBitCount % BitsPerByte);
		byteCount = (myEncodedBitCount / BitsPerByte);
		//if(myEncodedBitCount%BitsPerByte)
		//{
		//	byteCount++;
		//}

		//sendEncodedSamples(winningEncodedStream, encodedSize);
		sendEncodedSamples(winningEncodedStream, encodedSize);
		cout << " Byte Index=" << byteCount << " additionalBits=" << additionalBits << "...";
		//sendEncodedSamples(encodedStream, lastByte, encodedSize);
		previousEncodedStream = winningEncodedStream;

		t3_intermediate = getTimestamp();

		lastWinningEncodedLength = winningEncodedStream.size();

	}

	timestamp_t t3 = getTimestamp();

	cout << "\nRepresentative intermediate Encoding processing times ==> " << fixed
			<< "\n(intermediate t0-t1): " << fixed
			<< getSecondsDiff(t0_intermediate, t1_intermediate) << " seconds"
			<< "\n(intermediate t1-t2): " << fixed
			<< getSecondsDiff(t1_intermediate, t2_intermediate) << " seconds"
			<< "\n(intermediate t2-t3): " << fixed
			<< getSecondsDiff(t2_intermediate, t3_intermediate) << " seconds\n" << endl;

	cout << "Encoding processing time ==> " << fixed << getSecondsDiff(t2, t3) << " seconds"
			<< endl;

	//**********************************************
	//exit(0);
	//**********************************************

}


void Sensor::sendHeader()
{
    // Note: Header is not completely populated for all defined parameters.
    // Only what is applicable to the selected test raw data to
    // identify identical information. Also, probably not the most
    // clean way to fill in fields.

    CompressedHeader header = {0};

    // Collect the structure data
    header.xDimension = myXDimension;
    header.yDimension = myYDimension;
    header.zDimension = myZDimension;
    bigEndianVersusLittleEndian(header.xDimension);
    bigEndianVersusLittleEndian(header.yDimension);
    bigEndianVersusLittleEndian(header.zDimension);

    //----------------------------------------------------------------------------
    bool signedSamples(false);
    bool bsq(true);
    header.signSampDynRangeBsq1 |= signedSamples;         header.signSampDynRangeBsq1 <<= 2; // reserved
                                                          header.signSampDynRangeBsq1 <<= 4;
    header.signSampDynRangeBsq1 |= (DynamicRange & 0xf);  header.signSampDynRangeBsq1 <<= 1;
    header.signSampDynRangeBsq1 |= bsq;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    bool blockType(true);

    header.wordSizEncodeMethod |= blockType;  header.wordSizEncodeMethod <<= 2;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------

    header.predictBandMode |= UserInputPredictionBands;  header.predictBandMode <<= 2;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    bool neighborSum(true);

    header.neighborRegSize |= neighborSum;  header.neighborRegSize <<= 7;
    header.neighborRegSize |= RegisterSize;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    header.predictWeightResInit |= (PredictionWeightResolution - 4);  header.predictWeightResInit <<= 4;

    unsigned int scaledWeight = log2(PredictionWeightInterval);
    header.predictWeightResInit |= (scaledWeight - 4);
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    header.predictWeightInitFinal |= (PredictionWeightInitial + 6);  header.predictWeightInitFinal <<= 4;

    header.predictWeightInitFinal |= (PredictionWeightFinal + 6);
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    header.blockSizeRefInterval |= (0x40);  // Block size 32 flag

    ushort refInterval(ReferenceInterval);
    bigEndianVersusLittleEndian(refInterval);

    header.blockSizeRefInterval |= refInterval;
    //----------------------------------------------------------------------------

    boost::dynamic_bitset<unsigned char> packedData;
    packCompressedData(header.userData, packedData);                // byte 0
    packCompressedData(header.xDimension, packedData);              // bytes 1,2
    packCompressedData(header.yDimension, packedData);              // bytes 3,4
    packCompressedData(header.zDimension, packedData);              // bytes 5,6
    packCompressedData(header.signSampDynRangeBsq1, packedData);    // byte 7
    packCompressedData(header.bsq, packedData);                     // bytes 8,9
    packCompressedData(header.wordSizEncodeMethod, packedData);     // bytes 10,11
    packCompressedData(header.predictBandMode, packedData);         // byte 12
    packCompressedData(header.neighborRegSize, packedData);         // byte 13
    packCompressedData(header.predictWeightResInit, packedData);    // byte 14
    packCompressedData(header.predictWeightInitFinal, packedData);  // byte 15
    packCompressedData(header.predictWeightTable, packedData);      // byte 16
    packCompressedData(header.blockSizeRefInterval, packedData);    // bytes 17,18


    size_t bitsPerBlock = packedData.bits_per_block;
    size_t numBlocks = packedData.num_blocks();


    writeCompressedData(packedData);

}

void Sensor::sendEncodedSamples(boost::dynamic_bitset<> &encodedStream, unsigned int encodedLength)
//void Sensor::sendEncodedSamples(boost::dynamic_bitset<> &encodedStream, unsigned char &lastByte, unsigned int encodedLength)
{
	bool endFlag(false);

	// if 0, then whatever is there can be appended and sent
	if(encodedLength == 0)
	{
		endFlag = true;
	}


	size_t bytes = encodedStream.size() / BitsPerByte;
	unsigned int previousSize = encodedStream.size();

	if (encodedStream.size() % BitsPerByte)
	{
		bytes++;
		unsigned int newSize = bytes * BitsPerByte;
		encodedStream.resize(newSize);
		encodedStream <<= (newSize - previousSize);
	}

	// this does two things - (1) changes the block size from ulong to
	// unsigned char and (2) reverses the byte order
	// Note that this algorithm was largely arrived at empirically. Looking
	// at the data to see what is correct. Keep this in mind when defining
	// architectural decisions, and when there may exist reluctance
	// after prototyping activities.

	boost::dynamic_bitset<unsigned char> convertedStream(encodedStream.size());
	for (int byteIndex = 0; byteIndex < bytes; byteIndex++)
	{
		int targetByte = bytes - byteIndex - 1;
		int sourceByte = byteIndex;

		for (int bitIndex = 0; bitIndex < BitsPerByte; bitIndex++)
		{
			int targetBit = (targetByte * BitsPerByte) + bitIndex;
			int sourceBit = (sourceByte * BitsPerByte) + bitIndex;

			convertedStream[targetBit] = encodedStream[sourceBit];
		}

	}

	//writeCompressedData(convertedStream, encodedStream.size(), true);
	writeCompressedData(convertedStream, previousSize, true);

	//*****************************************************
//	static int debugCount(0);
//	if(debugCount >=3)
//	{
//	   myEncodedStream.close(); // :TODO: temporary test
//	   exit(0);
//	}
//	debugCount++;
    //*****************************************************


}

void Sensor::writeCompressedData(boost::dynamic_bitset<unsigned char> &packedData, size_t bitSize, bool flag)
{
    
	// A non-default bit size might be specified, but this must be adjusted to the nearest
	// full bit
	if (!bitSize)
	{
		bitSize=packedData.size();
	}

    size_t numberOfBytes = bitSize/BitsPerByte;
    //if(bitSize % BitsPerByte)
    //{
    	//numberOfBytes++;
    //}


    vector<unsigned char> packedDataBlocks(packedData.num_blocks());

    //populate vector blocks
    boost::to_block_range(packedData, packedDataBlocks.begin());

    cout << "Writing Byte:" << mySource->getBytesWritten() << endl;

    //write out each block
    for (vector<unsigned char>::iterator it =
            packedDataBlocks.begin(); it != packedDataBlocks.end(); ++it)
    {
        //retrieves block and converts it to a char*
        mySource->sendEncodedData(reinterpret_cast<char*>(&*it));

        // if we've written the targeted number of bytes
        // return
        numberOfBytes--;
        if(!numberOfBytes)
        {
        	break;
        }
    }
}

bool Sensor::getLastByte(unsigned char *lastByte)
{
    // Get the last byte written, and in some cases, reset the file pointer to the one previous

    bool partialByteFlag(false);
    unsigned int encodedLength = myEncodedBitCount / BitsPerByte;

    int additionalBits = myEncodedBitCount % BitsPerByte;
    if(additionalBits)
    {

        encodedLength++;

        unsigned char* encodedPtr = mySource->getEncodedData();

        *lastByte = encodedPtr[encodedLength-1];
        
        partialByteFlag = true;
    }

    if(partialByteFlag)
    {
        mySource->setNextInsertionByte(encodedLength-1);
    }

    return partialByteFlag;

}
