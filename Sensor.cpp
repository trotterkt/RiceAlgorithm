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

    long totalSamples = myXDimension*myYDimension*myZDimension;

    for(blockIndex = 0; blockIndex<totalSamples; blockIndex+=32)
    {
        // Reset for each capture of the winning length
        myWinningEncodedLength = (unsigned int) -1;

        // Loop through each one of the possible encoders
        for (std::vector<AdaptiveEntropyEncoder*>::iterator iteration = myEncoderList.begin();
                iteration != myEncoderList.end(); ++iteration)
        {
            // 1 block at a time
            (*iteration)->setSamples(&residualsPtr[blockIndex]);

            memset(encodedBlock, 0, BlockSize * sizeof(unsigned int));  // is this right?

            CodingSelection selection; // This will be most applicable for distinguishing FS and K-split

            unsigned int encodedLength = (*iteration)->encode(encodedBlock, encodedStream, selection);

            // This basically determines the winner
            if (encodedLength < myWinningEncodedLength)
            {
                *this = *(*iteration);
                myWinningEncodedLength = encodedLength;
                winningSelection = selection;
            }
//
            //cout << "Finished Encoder Iteration:" << (iteration - myEncoderList.begin()) << endl;
        }

        //CodingSelection selection = (*winningIteration)->getSelection();
        cout << "And the Winner is: " << int(winningSelection) << " of code length: " << myWinningEncodedLength << " on Block Sample [" << blockIndex << "]" << endl;

        //:TODO: Figure this out
        //sendEncodedSamples(*(*winningIteration));
        sendEncodedSamples(encodedStream);


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

//    //:TODO: test this out for the encoded data
//    //*******************************************************
//    boost::dynamic_bitset<unsigned char> packedData2;
//
//
//    boost::dynamic_bitset<ulong> method(sizeof(ulong)*8, 0xB);
//    method <<= 22;
//    boost::dynamic_bitset<ulong> encodedData(sizeof(ulong)*8, 0x01);
//    encodedData |= method;
//    encodedData <<= ((sizeof(ulong)*8) - 26);
//
//    ulong data4 = encodedData.to_ulong();
//    bigEndianVersusLittleEndian(data4);
//
//    packCompressedData(data4, packedData2);
//    writeCompressedData(packedData2, 26);
//
//    //*******************************************************
//
//	myEncodedStream.close(); // :TODO: temporary test
}

void Sensor::sendEncodedSamples(boost::dynamic_bitset<> &encodedStream)
{
/*
	// see Lossless Data Compression, Blue Book, sec 5.1.2
	short codeOptionBits(0);
	switch(encoder.getSelection())
	{
		case ZeroBlockOpt:
		case SecondExtensionOpt:
			codeOptionBits = CodeOptionBitFieldZeroOrSecEx;
			break;

		default:
			codeOptionBits = CodeOptionBitFieldFundamentalOrNoComp;
			break;
	}


    int numberOfBits = (32 * sizeof(ushort) * 8) + codeOptionBits;
*/
/*
    //:TODO: test this out for the encoded data
    //*******************************************************
    boost::dynamic_bitset<unsigned char> packedData2;


    boost::dynamic_bitset<ulong> method(sizeof(ulong)*8, 0xB);
    method <<= 22;
    boost::dynamic_bitset<ulong> encodedData(sizeof(ulong)*8, 0x01);
    encodedData |= method;
    encodedData <<= ((sizeof(ulong)*8) - 26);

    ulong data4 = encodedData.to_ulong();
    bigEndianVersusLittleEndian(data4);

    packCompressedData(data4, packedData2);
    writeCompressedData(packedData2, 26);
*/
    //*******************************************************
	//boost::dynamic_bitset<unsigned char> encodedStream2(encodedStream);
	//encodedStream2 = encodedStream;



        size_t bytes = encodedStream.size()/BitsPerByte;
        if(encodedStream.size() % BitsPerByte)
        {
        	bytes++;
        }

        // this does three things - (1) changes the block size from ulong to
        // unsigned char (2) reverses the byte order, (3) any fill bits
        // are placed at the end, and (4) exits early for filled bits
        // Note that this algorithm was largely arrived at empirically. Looking
        // at the data to see what is correct. Keep this in mind when defining
        // architectural decisions, and when there may exist reluctance
        // after prototyping activities.
        size_t offset = (bytes*BitsPerByte)-encodedStream.size();

	    boost::dynamic_bitset<unsigned char> convertedStream(encodedStream.size());
	    for(int byteIndex=0; byteIndex<bytes; byteIndex++)
	    {
	    	int targetByte = bytes - byteIndex - 1;
	    	int sourceByte = byteIndex;

	    	for(int bitIndex=0; bitIndex<BitsPerByte; bitIndex++)
	    	{
	    		int targetBit = (targetByte*BitsPerByte) + bitIndex;
	    		int sourceBit = (sourceByte*BitsPerByte) + bitIndex;

		    	convertedStream[targetBit] =  encodedStream[sourceBit];
		    	//cout << convertedStream[targetBit + offset];
		    	cout << targetBit << ", ";

//		    	if(targetBit == offset)
//		    	{
//		    		break;
//		    	}
	    	}

	    }
        cout << endl;
	    // it appears shifting does not work as expected hear
	    //convertedStream >>= ((bytes*BitsPerByte)-convertedStream.size());


/*
	    vector<unsigned char> convertedDataBlocks(convertedStream.num_blocks());

	    //populate vector blocks
	    boost::to_block_range(convertedStream, convertedDataBlocks.begin());

	    uint8_t buffer[bytes];
        int index(0);
	    //write out each block
	    for (vector<unsigned char>::iterator it =
	    		convertedDataBlocks.begin(); it != convertedDataBlocks.end(); ++it)
	    {

	    	buffer[index] = *it;
	    	index++;
	    }

	    // Essentially need an endian conversion
//		memcpy(buffer, &numberToTranslate, bytes);
//
		int i = 0;
		int j = bytes - 1;

		while(i<j)
		{
			std::swap(buffer[i], buffer[j]);
			i++;
			j--;
		}
//		memcpy(&numberToTranslate, buffer, bytes);

*/

    writeCompressedData(convertedStream);










	myEncodedStream.close(); // :TODO: temporary test

/*
    // create on byte boundaries
    double remainder(0), integer(0);
    remainder = modf((numberOfBits/8), &integer);

    int additionalBits = remainder * 8;
    numberOfBits += additionalBits;

    int currentBitLocation(0);

    currentBitLocation = currentBitLocation - codeOptionBits;

    currentBitLocation = currentBitLocation + codeOptionBits;

    ushort* residualsPtr = myPreprocessor.getResiduals();

	int length[32];

	// decide how many zeros are appropriate for split sequence
	// apply this in reverse order
    for(int i = 0; i < 32; i++)
    {
    	int shiftOne = (residualsPtr)[i] >>
    			       int(encoder.getSelection()); // this is the number of zeros before a one
    	length[31-i] = shiftOne+1;
    }

    unsigned long lsbWord(0xb);
    unsigned long msbWord(0);

    int totalBitShift(0);

    // construct k-split sequence
    for(int i = 31; i >= 0; i--)
    {
        // whatever length[i] is in the msb bits, on the ksbWord,
    	// copy it to the first of the high-order word 4 --> 15 (2^4)-1
    	double shiftBits = length[i];
    	unsigned long mask = (exp2(shiftBits) - 1);
    	mask <<= ((sizeof(unsigned long)*8) - length[i]);
    	unsigned long tempMsbWord = lsbWord & mask;
    	tempMsbWord >>= ((sizeof(unsigned long)*8) - length[i]);
        msbWord <<= length[i];
        msbWord |= tempMsbWord;

        lsbWord = lsbWord << length[i];
        lsbWord = lsbWord | 0x1;
        totalBitShift += length[i];
    }

    // :KLUDGE: need to address endian for non-standard
    // data type
    bigEndianVersusLittleEndian(lsbWord);
    bigEndianVersusLittleEndian(msbWord);


    for(int i = 0; i < 32; i++)
    {
    	ushort code = mySamples[i] >> int(encoder.getSelection());

    	std::bitset<516> code3(mySamples[i]);
    	code3 <<= (516 - codeOptionBits - ((i+1)*sizeof(ushort)*8));
    }
*/
}

void Sensor::writeCompressedData(boost::dynamic_bitset<unsigned char> &packedData, size_t bitSize)
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

}
