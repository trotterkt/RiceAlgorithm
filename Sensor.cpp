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


Sensor::Sensor(char* filename, unsigned int x, unsigned int y, unsigned int z) :
		mySamples(0), myXDimension(x), myYDimension(y), myZDimension(z), myPreprocessor(x, y, z), myWinningEncodedLength((unsigned int)-1)
{
	// Filename ignores extension :TODO: will need to isolate persistence later
	myFileStream << filename;

	mySampleStream.open((myFileStream.str() + ".raw").c_str(), ios::in | ios::binary);

    if (!mySampleStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

    myEncodedStream.open((myFileStream.str() + ".comp").c_str(), ios::out | ios::in | ios::binary | ios::trunc);

    if (!myEncodedStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

	// get the length of the file
    mySampleStream.seekg(0, ios::end);
	myLength = mySampleStream.tellg();
	mySampleStream.seekg(0, ios::beg);


    mySamples = reinterpret_cast<ushort*>(new ushort[myXDimension * myYDimension * myZDimension]);

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
}

Sensor::~Sensor()
{
    mySampleStream.close();
    myEncodedStream.close();

    delete[] mySamples;
}

ushort* Sensor::getSamples(uint scanNumber)
{
    unsigned short int buffer(0);
    static unsigned long readElements(0);

    streamsize sampleSize = sizeof(buffer);

    uint blockSize = myXDimension * myYDimension * myZDimension;

    // Skip ahead on the next scan
    readElements = ((scanNumber-1) * blockSize);


    //Now, instead, we are in the situation that only the exact D bits are specified for
    //every sample.
    //I read two bytes (16 bits) at a time, eventually eliminating the most
    //significant bits, in case the length of the residuals is smaller than 16 bits,
    //keeping the remaining bits for the next residual
    //I repeat until the input file is empty
    //while ((readElements * sampleSize) < (myLength/scanNumber))
    while (readElements < (blockSize*scanNumber))
    {
        mySampleStream.read(reinterpret_cast<char*>(&buffer), sampleSize);

        // This assumes the data is in BSQ format and we do not need to adjust the indexing
        mySamples[readElements] = buffer;

        readElements++;
    }

    // Determine if uneven number (1) of bytes left
    if ((myLength - mySampleStream.tellg()) == 1)
    {
    	mySampleStream.read(reinterpret_cast<char*>(&buffer), 1);
        mySamples[readElements] = buffer;
    }


    return mySamples;
}

void Sensor::process()
{
	// The goal hear is to form the telemetry of the data

	sendHeader();

    //myEncodedBitCount = 0;

	// :TODO: formalize this a little more
    myEncodedBitCount = 19*BitsPerByte; // Start with header length


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

    timestamp_t t0_intermediate, t1_intermediate, t2_intermediate, t3_intermediate;

    timestamp_t t0 = getTimestamp();

    // Should only need to get the residuals once for a given raw image set
    ushort* residualsPtr = myPreprocessor.getResiduals(mySamples);
    
    timestamp_t t1 = getTimestamp();

    cout << "Prediction processing time ==> " << fixed << getSecondsDiff(t0, t1) << " seconds"<< endl;


    timestamp_t t2 = getTimestamp();

    int blockIndex(0);
    unsigned int encodedLength(0);

    long totalSamples = myXDimension*myYDimension*myZDimension;

    //:TODO: This is one of the 1st places where we will start looking
    // at applying Amdahl's Law!!!

    for(blockIndex = 0; blockIndex<totalSamples; blockIndex+=32)
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

            //encodedLength = (*iteration)->encode(encodedBlock, encodedStream, selection);
            encodedLength = (*iteration)->encode(encodedStream, selection);

            // This basically determines the winner
            if (encodedLength < myWinningEncodedLength)
            {
                *this = *(*iteration);
                myWinningEncodedLength = encodedLength;
                winningSelection = selection;

                encodedSize = (*iteration)->getEncodedBlockSize();
            }
        }

        t1_intermediate = getTimestamp();

        //cout << "And the Winner is: " << int(winningSelection) << " of code length: " << myWinningEncodedLength << " on Block Sample [" << blockIndex << "]" << endl;


        ushort partialBits = myEncodedBitCount % BitsPerByte;
        unsigned char lastByte(0);

        // When the last byte written is partial, as compared with the
        // total bits written, capture it so that it can be merged with
        // the next piece of encoded data
        if (getLastByte(lastByte))
		{
        	//cout << "Before partial appendage: " << encodedStream << endl;
        	unsigned int appendedSize = encodedStream.size()+partialBits;
			encodedStream.resize(appendedSize);
        	//cout << "Again  partial appendage: " << encodedStream << endl;

			boost::dynamic_bitset<> lastByteStream(encodedStream.size(), lastByte);
			lastByteStream <<= (encodedStream.size() - partialBits);
			encodedStream |= lastByteStream;
        	//cout << "After partial appendage : " << encodedStream << endl;
		}

        myEncodedBitCount += (myWinningEncodedLength + CodeOptionBitFieldFundamentalOrNoComp);

        t2_intermediate = getTimestamp();

        static unsigned int lastWinningEncodedLength(0);

        sendEncodedSamples(encodedStream, encodedSize);

        getLastByte(lastByte);

        t3_intermediate = getTimestamp();

        lastWinningEncodedLength = encodedStream.size();

    }

    timestamp_t t3 = getTimestamp();


    cout << "\nRepresentative intermediate Encoding processing times ==> " << fixed
            << "\n(intermediate t0-t1): " << fixed << getSecondsDiff(t0_intermediate, t1_intermediate) << " seconds"
            << "\n(intermediate t1-t2): " << fixed << getSecondsDiff(t1_intermediate, t2_intermediate) << " seconds"
            << "\n(intermediate t2-t3): " << fixed << getSecondsDiff(t2_intermediate, t3_intermediate) << " seconds\n" << endl;

    cout << "Encoding processing time ==> " << fixed << getSecondsDiff(t2, t3) << " seconds"<< endl;

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
{
	bool endFlag(false);

	// if 0, then whatever is there can be appended and sent
	if(encodedLength == 0)
	{
		endFlag = true;
	}


	size_t bytes = encodedStream.size() / BitsPerByte;
	if (encodedStream.size() % BitsPerByte)
	{
		unsigned int previousSize = encodedStream.size();
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

	writeCompressedData(convertedStream, encodedStream.size(), true);

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
    if(bitSize % BitsPerByte)
    {
    	numberOfBytes++;
    }


    vector<unsigned char> packedDataBlocks(packedData.num_blocks());

    //populate vector blocks
    boost::to_block_range(packedData, packedDataBlocks.begin());

    //write out each block
    for (vector<unsigned char>::iterator it =
            packedDataBlocks.begin(); it != packedDataBlocks.end(); ++it)
    {

        //retrieves block and converts it to a char*
        myEncodedStream.write(reinterpret_cast<char*>(&*it), sizeof(unsigned char));


        // if we've written the targeted number of bytes
        // return
        numberOfBytes--;
        if(!numberOfBytes)
        {
        	break;
        }
    }

    // since the stream is bidirectional, we must reposition the file pointer
    // after every read to write, or visa versa
    myEncodedStream.seekg(0, ios::end);
}

bool Sensor::getLastByte(unsigned char &lastByte)
{
    // Get the last byte written, and in some cases, reset the file pointer to the one previous

    bool partialByteFlag(false);

    if(myEncodedBitCount % BitsPerByte)
    {
        myEncodedStream.seekg ((myEncodedBitCount % BitsPerByte), ios::beg);
        myEncodedStream.get(*reinterpret_cast<char *>(&lastByte));

        partialByteFlag = true;
    }

    // since the stream is bidirectional, we must reposition the file pointer
    // after every read to write, or visa versa
    unsigned int putByte = myEncodedBitCount / BitsPerByte;
    myEncodedStream.seekp (putByte, ios::beg);

    return partialByteFlag;

}
