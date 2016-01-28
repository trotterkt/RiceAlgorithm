/*
 * Sensor.cpp
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#include <Sensor.h>
#include <iostream>

using namespace std;
using namespace RiceAlgorithm;

Sensor::Sensor(char* filename, unsigned int x, unsigned int y, unsigned int z) :
		mySamples(0), myXDimension(x), myYDimension(y), myZDimension(z), myPreprocessor(x, y, z)
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


    myEncoderList.push_back(noComp);
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

	myPreprocessor.readSamples(mySamples);


	unsigned int encodedBlock[BlockSize];

	//:TODO: Nest this loop in another and iterate over the next residual block

	for (std::vector<AdaptiveEntropyEncoder*>::iterator iteration = myEncoderList.begin();
		 iteration != myEncoderList.end(); ++iteration)
	{
		// 1 block at a time
		(*iteration)->setSamples(myPreprocessor.getResiduals());

		CodingSelection selection; // This will be most applicable for distinguishing FS and K-split
	    unsigned int length = (*iteration)->encode(encodedBlock, selection);

	    getWinner(encodedBlock, length, selection, (iteration == myEncoderList.end()));

	    cout << "Block" << endl;
	}

}

void Sensor::getWinner(unsigned int* encodedBlock,
		               ushort codeLength,
		               RiceAlgorithm::CodingSelection selection,
		               bool lastType)
{
    // determine winner for current encoded block
	// use where we are in the iteration

	//:TODO: if this or other methods include local static variables
	// maybe that should be avoided in the architectural description for
	// GPGPU

	// copy the block if smaller to the stream for send, along with
	// the code selection
	memcpy(myEncodedStream, encodedBlock, (codeLength * sizeof(unsigned int)));

}

void Sensor::createHeader()
{
	// Write out the image dimensions - for my purposes these will
	// be constants

}
