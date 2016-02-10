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

    myEncodedStream.open((myFileStream.str() + ".comp").c_str(), ios::out | ios::in | ios::binary);

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
    // Note: Header is not completely populated for all defined parameters.
    // Only what is applicable to the selected test raw data to
    // identify identical information. Also, probably not the most
    // clean way to fill in fields.

    CompressedHeader header = {0};

    // Collect the structure data
    *reinterpret_cast<short*>(&header.xDimension) = myXDimension;
    *reinterpret_cast<short*>(&header.yDimension) = myYDimension;
    *reinterpret_cast<short*>(&header.zDimension) = myZDimension;
    bigEndianVersusLittleEndian(*reinterpret_cast<ushort*>(&header.xDimension));
    bigEndianVersusLittleEndian(*reinterpret_cast<ushort*>(&header.yDimension));
    bigEndianVersusLittleEndian(*reinterpret_cast<ushort*>(&header.zDimension));

    //----------------------------------------------------------------------------
    bool signedSamples(false);
    bool bsq(true);
    header.combinedField1 |= signedSamples;         header.combinedField1 <<= 2; // reserved
                                                    header.combinedField1 <<= 4;
    header.combinedField1 |= (DynamicRange & 0xf);  header.combinedField1 <<= 1;
    header.combinedField1 |= bsq;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    bool blockType(true);

    header.combinedField2[0] |= blockType;  header.combinedField2[0] <<= 2;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------

    header.combinedField3 |= UserInputPredictionBands;  header.combinedField3 <<= 2;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    bool neighborSum(true);

    header.combinedField4 |= neighborSum;  header.combinedField4 <<= 7;
    header.combinedField4 |= RegisterSize;
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    header.combinedField5 |= (PredictionWeightResolution - 4);  header.combinedField5 <<= 4;

    unsigned int scaledWeight = log2(PredictionWeightInterval);
    header.combinedField5 |= (scaledWeight - 4);
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    header.combinedField6 |= (PredictionWeightInitial + 6);  header.combinedField6 <<= 4;

    header.combinedField6 |= (PredictionWeightFinal + 6);
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    header.combinedField8 |= (0x40);

    header.combinedField8 |= (0x80);
    //----------------------------------------------------------------------------

    boost::dynamic_bitset<unsigned char> filter;

    // Since the data is of uneven types, all are read in as single bytes
    filter.append(*reinterpret_cast<char*>(&header.userData));
    filter.append(*reinterpret_cast<char*>(&header.xDimension[0]));
    filter.append(*reinterpret_cast<char*>(&header.xDimension[1]));
    filter.append(*reinterpret_cast<char*>(&header.yDimension[0]));
    filter.append(*reinterpret_cast<char*>(&header.yDimension[1]));
    filter.append(*reinterpret_cast<char*>(&header.zDimension[0]));
    filter.append(*reinterpret_cast<char*>(&header.zDimension[1]));
    filter.append(*reinterpret_cast<char*>(&header.combinedField1));
    filter.append(*reinterpret_cast<char*>(&header.bsq[0]));
    filter.append(*reinterpret_cast<char*>(&header.bsq[1]));
    filter.append(*reinterpret_cast<char*>(&header.combinedField2[0]));
    filter.append(*reinterpret_cast<char*>(&header.combinedField2[1]));
    filter.append(*reinterpret_cast<char*>(&header.combinedField3));
    filter.append(*reinterpret_cast<char*>(&header.combinedField4));
    filter.append(*reinterpret_cast<char*>(&header.combinedField5));
    filter.append(*reinterpret_cast<char*>(&header.combinedField6));
    filter.append(*reinterpret_cast<char*>(&header.combinedField7));
    filter.append(*reinterpret_cast<char*>(&header.combinedField8));

    size_t bitsPerBlock = filter.bits_per_block;
    size_t numBlocks = filter.num_blocks();

    vector<unsigned char> filterBlocks(filter.num_blocks());

    //populate vector blocks
    boost::to_block_range(filter, filterBlocks.begin());

    //write out each block
    for (vector<unsigned char>::iterator it =
            filterBlocks.begin(); it != filterBlocks.end(); ++it)
    {
        //retrieves block and converts it to a char*
        myEncodedStream.write(reinterpret_cast<char*>(&*it), sizeof(unsigned char));
    }




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

