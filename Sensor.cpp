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
#include <DebuggingParameters.h>
#include <ShiftFunctions.h>

#ifdef DEBUG
#include <fstream>
#endif

using namespace std;
using namespace RiceAlgorithm;

Sensor::Sensor(ImagePersistence* image, unsigned int x, unsigned int y, unsigned int z) :
		mySource(image), mySamples(0), myXDimension(x), myYDimension(y), myZDimension(z), myPreprocessor(
				x, y, z), myWinningEncodedLength((unsigned int) -1)
{

	mySamples = mySource->getSampleData(1); //:TODO: only one scan, need to address multiple

	// Create the encoding types
	size_t bufferSize = myXDimension * myYDimension * myZDimension;

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

	#ifdef DEBUG
    std::ofstream residualsStream;
    residualsStream.open("residuals.bin", ios::out | ios::in | ios::binary | ios::trunc);

    if (!residualsStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

    residualsStream.write(reinterpret_cast<char*>(residualsPtr), (1024*1024*6*2));
    residualsStream.close();
	#endif


	timestamp_t t1 = getTimestamp();

	cout << "Prediction processing time ==> " << fixed << getSecondsDiff(t0, t1) << " seconds"
			<< endl;

	timestamp_t t2 = getTimestamp();

	int blockIndex(0);
	unsigned int encodedLength(0);
	ulong count(0);

	long totalSamples = myXDimension * myYDimension * myZDimension;

	vector<unsigned char> completeEncoding;
	completeEncoding.reserve(totalSamples*sizeof(unsigned char));

	 // For a running total of just the encoded data (no header)
	 ulong currentEncodedSize(0);

	//:TODO: This is one of the 1st places where we will start looking
	// at applying Amdahl's Law!!!

	//totalSamples = 320; // Debugging

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

			#ifdef DEBUG
						//******************************
						//if(blockIndex > 32)
						//  break; // debugging
						//******************************
			#endif
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
				#ifdef DEBUG
                    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
                       ((count >= LowerRange2) && (count <= UpperRange2)))
                            cout << "And the Winner is: K" << int(winningSelection - 1) << " of code length: "
                            << myWinningEncodedLength << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]"
                            << ", count=" << count << endl;
				#endif
				break;

			case RiceAlgorithm::SecondExtensionOpt:
				#ifdef DEBUG
                    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
                       ((count >= LowerRange2) && (count <= UpperRange2)))
                            cout << "And the Winner is: 2ndEXT of code length: " << myWinningEncodedLength
                            << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]" << ", count=" << count << endl;
				#endif
				break;

			case RiceAlgorithm::ZeroBlockOpt:
				#ifdef DEBUG
                    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
                       ((count >= LowerRange2) && (count <= UpperRange2)))
                            cout << "And the Winner is: ZEROBLOCK of code length: " << myWinningEncodedLength
                            << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]" << ", count=" << count << endl;
				#endif
				break;

			case RiceAlgorithm::NoCompressionOpt:
				#ifdef DEBUG
                    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
                       ((count >= LowerRange2) && (count <= UpperRange2)))
                            cout << "And the Winner is: NOCOMP of code length: " << myWinningEncodedLength
                            << " (total=" << myEncodedBitCount << ") on Block Sample [" << blockIndex << "]" << ", count=" << count << endl;
				#endif

				break;

			default:
				cout << "Unanticipated encoding -- Exiting" << endl;
				exit(-1);

		}

		#ifdef DEBUG

        //*******************************************
        if(((count >= LowerRange1) && (count <= UpperRange1)) ||
           ((count >= LowerRange2) && (count <= UpperRange2)))
        {
            cout << "winning encoding==>" << winningEncodedStream << endl;
        }
        //*******************************************

		#endif

		ushort partialBits = myEncodedBitCount % BitsPerByte;
		unsigned char lastByte(0);

		// When the last byte written is partial, as compared with the
		// total bits written, capture it so that it can be merged with
		// the next piece of encoded data
		if (getLastByte(&lastByte))
		{
//			#ifdef DEBUG
//                if(((count >= LowerRange1) && (count <= UpperRange1)) ||
//                   ((count >= LowerRange2) && (count <= UpperRange2)))
//                        cout << "Before partial appendage: " << winningEncodedStream << endl;
//			#endif
//
//			unsigned int appendedSize = winningEncodedStream.size() + partialBits;
//			winningEncodedStream.resize(appendedSize);
//			boost::dynamic_bitset<> lastByteStream(winningEncodedStream.size(), ulong(lastByte));
//			lastByteStream <<= (winningEncodedStream.size() - BitsPerByte);
//			winningEncodedStream |= lastByteStream;
//
//			#ifdef DEBUG
//                if(((count >= LowerRange1) && (count <= UpperRange1)) ||
//                   ((count >= LowerRange2) && (count <= UpperRange2)))
//                        cout << "After partial appendage : " << winningEncodedStream << endl;
//			#endif
		}

        //myEncodedBitCount += (myWinningEncodedLength + CodeOptionBitFieldFundamentalOrNoComp);
        myEncodedBitCount += myWinningEncodedLength;

		t2_intermediate = getTimestamp();

		static unsigned int lastWinningEncodedLength(0);

		ulong byteCount(0);
		int additionalBits(myEncodedBitCount % BitsPerByte);
		byteCount = (myEncodedBitCount / BitsPerByte);

		//*************************************************************
		size_t numBlocks(0);

	    numBlocks = winningEncodedStream.num_blocks();

//		cout << "num bits = " << winningEncodedStream.size() << endl;
		vector<unsigned long> packedDataBlocks(numBlocks);

		boost::to_block_range(winningEncodedStream, packedDataBlocks.begin());
		std::reverse(packedDataBlocks.begin(), packedDataBlocks.end());


		// Make a char array to manipulate
        unsigned char* packedData = reinterpret_cast<unsigned char*>(new char[(packedDataBlocks.size() + 1)* sizeof(unsigned long)]);
        memset(packedData, 0, (packedDataBlocks.size() + 1)* sizeof(unsigned long));
        
        // not certain why endian swap on each element is nessassary
        for(int index=0; index<packedDataBlocks.size(); index++)
        {
           unsigned long temp = packedDataBlocks[index];
           bigEndianVersusLittleEndian(temp);
           packedDataBlocks[index] = temp;
	    }

        memcpy(packedData, &packedDataBlocks[0], (packedDataBlocks.size() * sizeof(unsigned long)));

	    //:KLUDGE: This is very much a kludge to fix the data available
        size_t dataBitLength = (packedDataBlocks.size() * sizeof(unsigned long) + 3) *  BitsPerByte; // Extra 3 bytes is to allow for shifting
	    adjustPackeDataPosition(packedData, dataBitLength);

	    //Packet size adjustment
	    float actualPacketBytes = ceil((encodedSize + partialBits +
	    		                   additionalBits +
	    		                   CodeOptionBitFieldFundamentalOrNoComp)/float(BitsPerByte));

		if(!completeEncoding.empty())
		{
	       shiftRight(packedData, dataBitLength, partialBits);

           unsigned char firstElement = packedData[0];

           currentEncodedSize += mySource->getEncodedDataSizes()[blockIndex/32 - 1];

           int elementLocation = (currentEncodedSize / BitsPerByte);

           if(currentEncodedSize % BitsPerByte)
           {
			   unsigned char lastElement = completeEncoding[elementLocation];

			   //cout << "firstElement=" << hex << int(firstElement) << ", lastElement=" << hex << int(lastElement) << dec << endl;

			   size_t shiftBits = mySource->getEncodedDataSizes()[blockIndex/32 - 1] % BitsPerByte;

			   lastElement |= (firstElement);
			   //cout << "Combined lastElement=" << hex << int(lastElement) << dec << " current completeEncoding size=" << completeEncoding.size() << endl;

			   // We might be off by one in size, if so, add a byte
			   if(elementLocation == completeEncoding.size())
			   {
				   lastElement = 0;
				   lastElement |= (firstElement);

				   completeEncoding.push_back(lastElement);
			   }

			   completeEncoding[elementLocation] = lastElement;

			   //int packetSize = (winningEncodedStream.size()/BitsPerByte) - 1;
			   int packetSize = (winningEncodedStream.size()/BitsPerByte);
//			   cout << "packetSize=" << packetSize << endl;
			   if(winningEncodedStream.size()%BitsPerByte)
			   {
//				   cout << " Remainder = " << (winningEncodedStream.size()%BitsPerByte) << " ... extra=" << additionalBits << endl;
				   packetSize++;
			   }

			   //***************************************************************
               // Add all of the remaining bytes
			   int packetStart = completeEncoding[completeEncoding.size()-1];
			   //packetSize = actualPacketBytes; // Try
			   //***************************************************************
//               if(count >= 4519)
//               {
//        	       cout << count << " Before completeEncoding==>" << hex << int(completeEncoding[packetStart-1]) << " " << int(completeEncoding[packetStart]) << " ... " << int(completeEncoding[completeEncoding.size()-3]) << " " << int(completeEncoding[completeEncoding.size()-2]) << " " << int(completeEncoding[completeEncoding.size()-1]) << dec << endl;
//               }
//
			   completeEncoding.insert(completeEncoding.end(), &packedData[1],  &packedData[1]+packetSize-1);
//
//			   if(count >= 4519)
//               {
//        	       cout << count << " After completeEncoding==>" << hex << int(completeEncoding[packetStart-1]) << " " << int(completeEncoding[packetStart]) << " ... " << int(completeEncoding[completeEncoding.size()-3]) << " " << int(completeEncoding[completeEncoding.size()-2]) << " " << int(completeEncoding[completeEncoding.size()-1]) << dec << endl;
//               }

           }
           else
           {
               // Add all of the bytes
               completeEncoding.insert(completeEncoding.end(), &packedData[0],  &packedData[0]+(winningEncodedStream.size()/BitsPerByte));
           }


		   delete []packedData;
		}
        else
        {
            completeEncoding.insert(completeEncoding.end(), &packedData[0],  &packedData[0]+(winningEncodedStream.size()/BitsPerByte));
        }

		//cout << "Complete Encoding==>" << hex << int(completeEncoding[0]) << " " <<  int(completeEncoding[1]) << " ... " <<  int(completeEncoding[completeEncoding.size()-1]) << dec << endl;

		//*************************************************
		int packetBitLength = encodedSize;
		int byte;
		int bit;
		RiceAlgorithm::CodingSelection currentSelection;
		getExpectedNextPacketPosition(&completeEncoding[0], packetBitLength, byte, bit, count);
        //*************************************************

		//**************************************************************

		sendEncodedSamples(winningEncodedStream, encodedSize);

		#ifdef DEBUG
            if(((count >= LowerRange1) && (count <= UpperRange1)) ||
               ((count >= LowerRange2) && (count <= UpperRange2)))
                    cout << " Byte Index=" << byteCount << " additionalBits=" << additionalBits << "...";
		#endif

		previousEncodedStream = winningEncodedStream;

		t3_intermediate = getTimestamp();

		lastWinningEncodedLength = winningEncodedStream.size();


		// Debug
		if(count == 4521)
		{
//			ulong numberOfBlocks = completeEncoding.size();
//		    mySource->writeEncodedData(&completeEncoding[0], numberOfBlocks);

//			break;
		}

	}

	ulong numberOfBlocks = completeEncoding.size();
    mySource->writeEncodedData(&completeEncoding[0], numberOfBlocks);

	timestamp_t t3 = getTimestamp();

	ofstream test("test.bin", ios::out | ios::binary | ios::trunc);
	test.write((char*)&completeEncoding[0], 10000);
	test.close();

	cout << "\nRepresentative intermediate Encoding processing times ==> " << fixed
			<< "\n(intermediate t0-t1): " << fixed
			<< getSecondsDiff(t0_intermediate, t1_intermediate) << " seconds"
			<< "\n(intermediate t1-t2): " << fixed
			<< getSecondsDiff(t1_intermediate, t2_intermediate) << " seconds"
			<< "\n(intermediate t2-t3): " << fixed
			<< getSecondsDiff(t2_intermediate, t3_intermediate) << " seconds\n" << endl;

	cout << "Encoding processing time ==> " << fixed << getSecondsDiff(t2, t3) << " seconds"
			<< endl;
}

void Sensor::sendHeader()
{
	// Note: Header is not completely populated for all defined parameters.
	// Only what is applicable to the selected test raw data to
	// identify identical information. Also, probably not the most
	// clean way to fill in fields.

	CompressedHeader header = { 0 };

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
	header.signSampDynRangeBsq1 |= signedSamples;
	header.signSampDynRangeBsq1 <<= 2; // reserved
	header.signSampDynRangeBsq1 <<= 4;
	header.signSampDynRangeBsq1 |= (DynamicRange & 0xf);
	header.signSampDynRangeBsq1 <<= 1;
	header.signSampDynRangeBsq1 |= bsq;
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	bool blockType(true);

	header.wordSizEncodeMethod |= blockType;
	header.wordSizEncodeMethod <<= 2;
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------

	header.predictBandMode |= UserInputPredictionBands;
	header.predictBandMode <<= 2;
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	bool neighborSum(true);

	header.neighborRegSize |= neighborSum;
	header.neighborRegSize <<= 7;
	header.neighborRegSize |= RegisterSize;
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	header.predictWeightResInit |= (PredictionWeightResolution - 4);
	header.predictWeightResInit <<= 4;

	unsigned int scaledWeight = log2(PredictionWeightInterval);
	header.predictWeightResInit |= (scaledWeight - 4);
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	header.predictWeightInitFinal |= (PredictionWeightInitial + 6);
	header.predictWeightInitFinal <<= 4;

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

	ulong headerSize = HeaderLength*BitsPerByte;
	writeCompressedData(packedData, headerSize);

}

void Sensor::sendEncodedSamples(boost::dynamic_bitset<> &encodedStream, unsigned int encodedLength)
{
	bool endFlag(false);

	// if 0, then whatever is there can be appended and sent
	if (encodedLength == 0)
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

    ulong totalBitCount(0);

    //cout << "****previousSize=" << previousSize << " encodedLength=" << encodedLength << endl;
    //totalBitCount = writeCompressedData(convertedStream, previousSize, true);
    totalBitCount = writeCompressedData(convertedStream, encodedLength, true);


	#ifdef DEBUG
	//*****************************************************
	//	static int debugCount(0);
	//	if(debugCount >=3)
	//	{
	//	   myEncodedStream.close(); // temporary test
	//	   exit(0);
	//	}
	//	debugCount++;
	//*****************************************************
	#endif
}

ulong Sensor::writeCompressedData(boost::dynamic_bitset<unsigned char> &packedData, size_t bitSize,
		bool flag)
{
	// Capture the bit sizes, except for header
	static ulong blockCount(0);


	if(blockCount)
	{
		mySource->setBlockBitSize((blockCount-1), bitSize);
	}
	blockCount++;


	static ulong totalBitCount(0);

	// A non-default bit size might be specified, but this must be adjusted to the nearest
	// full bit
	if (!bitSize)
	{
		bitSize = packedData.size();
	}


	long numberOfBytes = bitSize / BitsPerByte;

	if(bitSize % BitsPerByte)
	{
		numberOfBytes++;
	}

	//populate vector blocks
	vector<unsigned char> packedDataBlocks(packedData.num_blocks());

	//populate vector blocks
	boost::to_block_range(packedData, packedDataBlocks.begin());

//	ushort shiftedBits = totalBitCount % BitsPerByte;
//	//=============================================================================
//	// Merge first with the last byte
//	unsigned char firstByte = packedDataBlocks[0];
//	firstByte >>= shiftedBits;
//	firstByte |= lastByte;
//
//	//=============================================================================
//
//
//	// Shift the entire block before adding
//	if(shiftedBits)
//	{
//		unsigned int packetSize = bitSize;
//		unsigned char shiftedArray[65]; // room for 32 samples + the id
//		std::copy(packedDataBlocks.begin(), packedDataBlocks.end(), shiftedArray);
//		shiftLeft(shiftedArray, packetSize, shiftedBits);
//		unsigned int newArraySize = (bitSize - shiftedBits)/ BitsPerByte; // this should be whole bytes now
//		std::copy(shiftedArray, shiftedArray, packedDataBlocks.begin());
//		packedDataBlocks.resize(newArraySize);
//	}
//

	#ifdef DEBUG
	//	cout << "Writing Byte:" << mySource->getBytesWritten() << endl;
	#endif

	//write out each block
	for (vector<unsigned char>::iterator it = packedDataBlocks.begin();
			it != packedDataBlocks.end(); ++it)
	{
		if(it == packedDataBlocks.begin())
		{
			mySource->setEncodedByteLocation(totalBitCount/BitsPerByte);


			

			// bit position is the totalBitCount minus the Header bits
			if(totalBitCount)
			{
				// First time active is after the header. This is the reason from indexing from 2
				mySource->setEncodedBitLocation((blockCount-2), totalBitCount - (HeaderLength*BitsPerByte));
			}

			//continue;
		}

		//retrieves block and converts it to a char*
		mySource->sendEncodedData(reinterpret_cast<unsigned char*>(&*it));


		// if this is the last byte, capture it for consolidation
		// with next packet byte
//		if (numberOfBytes == 1)
//		{
//			getLastByte(&lastByte);
//			break;
//		}

		// if we've written the targeted number of bytes
		// return
		numberOfBytes--;
		if (numberOfBytes<0)
		{
			break;
		}
	}



	totalBitCount += bitSize;

	return totalBitCount;
}

bool Sensor::getLastByte(unsigned char *lastByte)
{

	// Get the last byte written, and in some cases, reset the file pointer to the one previous

	bool partialByteFlag(false);
	unsigned int encodedLength = myEncodedBitCount / BitsPerByte;

	int additionalBits = myEncodedBitCount % BitsPerByte;
	if (additionalBits)
	{

		encodedLength++;

		unsigned char* encodedPtr = mySource->getEncodedData();
		*lastByte = encodedPtr[encodedLength - 1];
		partialByteFlag = true;
	}

	if (partialByteFlag)
	{
		mySource->setNextInsertionByte(encodedLength - 1);
	}

	return partialByteFlag;
}

void Sensor::getExpectedNextPacketPosition(unsigned char* currentEncodingPtr, int packetBitLength, int &byte, int &bit, ulong count)
{
    static double currentTotalLength(0); // Starting after the header
    
    double endLength = currentTotalLength / double(BitsPerByte);
    int previousLocationByte = endLength;
    int previousLocationBit = (endLength - (long)endLength) * BitsPerByte;

    unsigned char selectionBytes[2]; // might span 2 bytes
    memcpy(selectionBytes, (currentEncodingPtr + previousLocationByte), 2);
    shiftLeft(selectionBytes, 16, previousLocationBit);
    selectionBytes[0] >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);

    RiceAlgorithm::CodingSelection selection;
    selection = CodingSelection(selectionBytes[0]);

    #ifdef DEBUG
    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
       ((count >= LowerRange2) && (count <= UpperRange2)))
            cout << "Current ID:K" << int(selection-1) << endl;
    #endif

    currentTotalLength += packetBitLength;
    endLength = currentTotalLength / double(BitsPerByte);
 
    // compare to what I expect
    if(myEncodedBitCount != (currentTotalLength + 152))
    {
        cout << "For count=" << count << " myEncodedBitCount=" << myEncodedBitCount << " while (currentTotalLength + 152)=" << (currentTotalLength + 152) << endl;
        exit(-1);
    }
}

