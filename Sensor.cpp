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

    myEncodedStream.open((myFileStream.str() + ".comp").c_str(), ios::out | ios::binary);

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
	createHeader();

	// The goal hear is to form the telemetry of the data

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

            unsigned int encodedLength = (*iteration)->encode(encodedBlock, selection);

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
    }

    //:TODO: Figure this out
	//createEncodingCodes(*(*winningIteration));
}


void Sensor::createHeader()
{
    // Alternate approch to the same thing as below -- but how do I extract
    // one byte at a time?
    //==================================================================
    //const size_t HeaderSize(156);
    const size_t HeaderSize(64);
    
    const size_t UserDataSize(8);
    const size_t DimensionSize(16);
    
    boost::dynamic_bitset<> header2(HeaderSize); // 19 bytes, 4 bits

    boost::dynamic_bitset<> userData(HeaderSize);
    boost::dynamic_bitset<> xDimension(HeaderSize, myXDimension);
    boost::dynamic_bitset<> yDimension(HeaderSize, myYDimension);
    boost::dynamic_bitset<> zDimension(HeaderSize, myZDimension);
    
    header2 |= userData; header2 <<= DimensionSize;
    header2 |= xDimension; header2 <<= DimensionSize;
    //header2 |= yDimension; header2 <<= DimensionSize;
    //header2 |= zDimension;
    
    size_t buffer = header2.to_ulong(); // this seems to work on linux
    size_t buffer2 = header2.num_blocks(); // this seems to work on linux

    
    //==================================================================

    CompressedHeader header = {0};

    *reinterpret_cast<short*>(&header.xDimension) = myXDimension;
    *reinterpret_cast<short*>(&header.yDimension) = myYDimension;
    *reinterpret_cast<short*>(&header.zDimension) = myZDimension;
    bigEndianVersusLittleEndian(*reinterpret_cast<short*>(&header.xDimension));
    bigEndianVersusLittleEndian(*reinterpret_cast<short*>(&header.yDimension));
    bigEndianVersusLittleEndian(*reinterpret_cast<short*>(&header.zDimension));

    header.combinedField1[2] = DynamicRange;


	// Write out the image dimensions - for my purposes these will
	// be constants

	// 1st write out 1 blank byte for user data
    //myEncodedStream.write(reinterpret_cast<char*>(&userData), sizeof(userData));

	// Then 2 bytes each for the x, y, and z dimensions
	//:TODO: For now, will just assume the header is 19 bytes -- will
	// define contents later
	//char primaryHeader[19];
	//memset(primaryHeader, 0, 19);


	myEncodedStream.write(reinterpret_cast<char*>(&header), sizeof(header));
	

	myEncodedStream.close(); // :TODO: temporary test
}

void Sensor::createEncodingCodes(RiceAlgorithm::AdaptiveEntropyEncoder& encoder)
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

