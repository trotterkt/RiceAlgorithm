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
	mySampleStream.open(filename, ios::out | ios::binary);

    if (!mySampleStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

	// get the length of the file
    mySampleStream.seekg(0, ios::end);
	myLength = mySampleStream.tellg();
	mySampleStream.seekg(0, ios::beg);


    mySamples = reinterpret_cast<u_short*>(new u_short[myXDimension * myYDimension * myZDimension]);

    // Create the encoding types
    size_t bufferSize = myXDimension*myYDimension*myZDimension;

    AdaptiveEntropyEncoder* noComp = new AdaptiveEntropyEncoder(bufferSize);
    SecondExtensionOption* secondExt = new SecondExtensionOption(bufferSize);
    ZeroBlockOption* zeroBlock = new ZeroBlockOption(bufferSize);
    SplitSequence* split = new SplitSequence(bufferSize);
//    SplitSequence* fundamentalSequence = new SplitSequence(bufferSize, K0); // FS is same as K=0
//    SplitSequence* split1 = new SplitSequence(bufferSize, K1);
//    SplitSequence* split2 = new SplitSequence(bufferSize, K2);
//    SplitSequence* split3 = new SplitSequence(bufferSize, K3);
//    SplitSequence* split4 = new SplitSequence(bufferSize, K4);
//    SplitSequence* split5 = new SplitSequence(bufferSize, K5);
//    SplitSequence* split6 = new SplitSequence(bufferSize, K6);
//    SplitSequence* split7 = new SplitSequence(bufferSize, K7);
//    SplitSequence* split8 = new SplitSequence(bufferSize, K8);
//    SplitSequence* split9 = new SplitSequence(bufferSize, K9);
//    SplitSequence* split10 = new SplitSequence(bufferSize, K10);
//    SplitSequence* split11 = new SplitSequence(bufferSize, K11);
//    SplitSequence* split12 = new SplitSequence(bufferSize, K12);
//    SplitSequence* split13 = new SplitSequence(bufferSize, K13);
//    SplitSequence* split14 = new SplitSequence(bufferSize, K14);

    myEncoderList.push_back(noComp);
    myEncoderList.push_back(secondExt);
    myEncoderList.push_back(zeroBlock);
    myEncoderList.push_back(split);

//    myEncoderList.push_back(fundamentalSequence);
//    myEncoderList.push_back(split1);
//    myEncoderList.push_back(split2);
//    myEncoderList.push_back(split3);
//    myEncoderList.push_back(split4);
//    myEncoderList.push_back(split5);
//    myEncoderList.push_back(split6);
//    myEncoderList.push_back(split7);
//    myEncoderList.push_back(split8);
//    myEncoderList.push_back(split9);
//    myEncoderList.push_back(split10);
//    myEncoderList.push_back(split11);
//    myEncoderList.push_back(split12);
//    myEncoderList.push_back(split13);
//    myEncoderList.push_back(split14);

}

Sensor::~Sensor()
{
    mySampleStream.close();
    delete[] mySamples;
}

u_short* Sensor::getSamples(uint scanNumber)
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

	for (std::vector<AdaptiveEntropyEncoder*>::iterator iteration = myEncoderList.begin();
		 iteration != myEncoderList.end(); ++iteration)
	{
		// 1 block at a time
		(*iteration)->setSamples(myPreprocessor.getResiduals());
	    (*iteration)->encode(encodedBlock);
	}

	// Get the winner

//	myEncoder.setSamples(myPreprocessor.getResiduals());
//	myEncoder.encode();

}

u_short* Sensor::getWinner()
{
	for (std::vector<AdaptiveEntropyEncoder*>::iterator iteration = myEncoderList.begin();
		 iteration != myEncoderList.end(); ++iteration)
	{

	}
}

