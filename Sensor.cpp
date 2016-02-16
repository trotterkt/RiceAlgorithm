/*
 * Sensor.cpp
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#include <Sensor.h>
#include <iostream>
#include <bitset>
#include <vector>
#include <math.h>
#include <limits.h>


using namespace std;
using namespace RiceAlgorithm;

ulong Sensor::bytesWritten(0);
ulong Sensor::bitsWritten(0);


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

	myPreprocessor.readSamples(mySamples);


	unsigned int encodedBlock[BlockSize];


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

    // Should only need to get the residuals once for a given raw image set
    ushort* residualsPtr = myPreprocessor.getResiduals();
    
    int blockIndex(0);
    unsigned int encodedLength(0);

    long totalSamples = myXDimension*myYDimension*myZDimension;

    for(blockIndex = 0; blockIndex<totalSamples; blockIndex+=32)
    {
        size_t encodedSize(0);

        // Reset for each capture of the winning length
        myWinningEncodedLength = (unsigned int) -1;

        // Loop through each one of the possible encoders
        for (std::vector<AdaptiveEntropyEncoder*>::iterator iteration = myEncoderList.begin();
                iteration != myEncoderList.end(); ++iteration)
        {
            encodedStream.clear();

            // 1 block at a time
            (*iteration)->setSamples(&residualsPtr[blockIndex]);

            memset(encodedBlock, 0, BlockSize * sizeof(unsigned int));  // is this right?

            CodingSelection selection; // This will be most applicable for distinguishing FS and K-split

            char lastByte = getLastByte();

            encodedLength = (*iteration)->encode(encodedBlock, encodedStream, selection, lastByte);

            // This basically determines the winner
            if (encodedLength < myWinningEncodedLength)
            {
                *this = *(*iteration);
                myWinningEncodedLength = encodedLength;
                winningSelection = selection;

                encodedSize = (*iteration)->getEncodedBlockSize();
            }
        }

        //CodingSelection selection = (*winningIteration)->getSelection();
        cout << "And the Winner is: " << int(winningSelection) << " of code length: " << myWinningEncodedLength << " on Block Sample [" << blockIndex << "]" << endl;

//        unsigned char lastByte = getLastByte();
//
//        static unsigned int lastWinningEncodedLength(0);
//
//        ushort partialBits = lastWinningEncodedLength % BitsPerByte;
//        encodedStream.resize(myWinningEncodedLength + partialBits);
//        encodedStream >>= partialBits;
//        boost::dynamic_bitset<> lastByteStream(partialBits, lastByte);
//        lastByteStream.resize(encodedStream.size());
//        lastByteStream <<= (encodedStream.size() - partialBits);
//        encodedStream |= lastByteStream;

        sendEncodedSamples(encodedStream, encodedSize);

//        lastWinningEncodedLength = myWinningEncodedLength;
//
//        lastByte = getLastByte();
//        cout << "lastByte" << lastByte << endl; // believe at the end of the first block, the last byte is 0, with 3 extra bits (49 bytes, 5 bits)
//                                                // so, starting with the last byte, shift the next block to the right by 5, then OR to last byte.
//                                                // When writing back out, change put file position by 1 byte from end.

    }

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

	//appendLsb(myFullEncodedStream, encodedStream);

	// if 0, then whatever is there can be appended and sent
	if(encodedLength == 0)
	{
		endFlag = true;
	}


	size_t bytes = encodedStream.size() / BitsPerByte;
	if (encodedStream.size() % BitsPerByte)
	{
		bytes++;
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
	static int debugCount(0);
	if(debugCount >=3)
	{
	   myEncodedStream.close(); // :TODO: temporary test
	   exit(0);
	}
	debugCount++;
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

unsigned char Sensor::getLastByte()
{
    // Get the last byte written, and in some cases, reset the file pointer to the one previous
    char lastByte(0);


    myEncodedStream.seekg (0, ios::end);
    int length = myEncodedStream.tellg();

    myEncodedStream.seekg (length-1, ios::beg);
    myEncodedStream.get(lastByte);

    // since the stream is bidirectional, we must reposition the file pointer
    // after every read to write, or visa versa
    myEncodedStream.seekp (0, ios::end);

    return lastByte;

}
