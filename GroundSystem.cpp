/*
 * GroundSystem.cpp
 *
 *  Created on: Mar 28, 2016
 *      Author: trotterkt
 */

#include <GroundSystem.h>
#include <Sensor.h>
#include <Endian.h>
#include <iostream>
#include <stdio.h>
#include <ShiftFunctions.h>
#include <DebuggingParameters.h>
#include <math.h>

#ifdef DEBUG
#include <fstream>
#endif

using namespace std;
using namespace RiceAlgorithm;


GroundSystem::GroundSystem(ImagePersistence* image) : mySource(image), myRawSamples(0)
{
    memset(&myHeader, 0, sizeof(CompressedHeader));
}

GroundSystem::~GroundSystem()
{
    if(myRawSamples)
    {
        delete [] myRawSamples;
        myRawSamples = 0;
    }
}

void GroundSystem::process(ushort* referenceResiduals)
{

	// This begins the decoding
	readHeader();

	// Having the raw sample dimensions from the header, allocate space for
	// the decoding
	const long NumberOfSamples(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension);

	myRawSamples = new ushort[NumberOfSamples];

	// Encoded data should begin right after the header (byte 19)

	// 1st grab the Encoded ID
	ulong totalEncodedLength(HeaderLength * BitsPerByte);

	unsigned int additionalBits(0);

	ushort* encodedBlockSizes = new ushort[(myHeader.xDimension *
	                                        myHeader.yDimension *
			                                myHeader.zDimension) / 32];
	ulong count(0);

	SplitSequence decodedSequence(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension);
	AdaptiveEntropyEncoder decodedNocomp(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension);

	//:TODO: It is possible this might better belong in an object that reverses the prediction
    ushort* residualsPtr = reinterpret_cast<ushort*>(new ushort[myHeader.xDimension * myHeader.yDimension * myHeader.zDimension]);

	// Read in one 32-sample block at a time (not on byte boundary)
	const long MaximumBlocks(NumberOfSamples / 32);
    ulong currentByteLocation = totalEncodedLength / BitsPerByte;

    CodingSelection nextSelection;
	unsigned char selectionByte(0);


	for (long blockIndex = 0; blockIndex < MaximumBlocks; blockIndex++)
	{
        count++;

    	selectionByte = 0;
    	memcpy(&selectionByte, mySource->getEncodedPacket(blockIndex), 1);
    	selectionByte >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);
    	selectionByte = selectionByte & 0xf;
    	nextSelection = CodingSelection(selectionByte);


		CodingSelection selection = nextSelection;

		#ifdef DEBUG
            if(((count >= LowerRange1) && (count <= UpperRange1)) ||
               ((count >= LowerRange2) && (count <= UpperRange2)))
                    switch(selection)
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
                                cout << "Encoding Selection = K" << int(selection-1) << ", currentByteLocation="
                                     << currentByteLocation << ", count=" << count << endl;
                            break;

                        case RiceAlgorithm::SecondExtensionOpt:
                                cout << "Found Winner is: 2ndEXT currentByteLocation="
                                     << currentByteLocation << ", count=" << count << endl;
                            break;

                        case RiceAlgorithm::ZeroBlockOpt:
                                cout << "Found Winner is: ZEROBLOCK currentByteLocation="
                                     << currentByteLocation << ", count=" << count << endl;
                            break;


                        case RiceAlgorithm::NoCompressionOpt:
                                            cout << "Found Winner is: NOCOMP currentByteLocation="
                                                 << currentByteLocation << ", count=" << count << endl;
                            break;

                        default:
                            cout << "Unanticipated encoding -- Exiting" << endl;
                            exit(-1);

                    }
		#endif


		// When the encoded zero-prefixed section ends, there should be a stream of 31 ones.
		// 1 binary bit for each of the 32 samples. This will be followed by 32 * k-select
		// value

		//:TRICKY:
		// count number ones in a byte
		//((i>>7)&1)+(i>>6)&1)+(i>>5)&1)+(i>>4)&1)+(i>>3)&1)+((i>>2)&1)+((i>>1)&1)+(i&1);

		// How do I pull out the encoded block lengths? See Sec. 6 of standard for CIP
		// alt. use 'a priori'

		// Assuming k-split type -
		// - count bits until 32-ones have been counted
		int encodeCount(0);

		// Account for encoded value not being on a byte boundary
        const unsigned int CopySize(32 * sizeof(ushort) + HeaderLength + 1); // Encoded data will be no larger than this
                                                                             // (include room for header and and next selection ID
                                                                             // since I now have the accurate length of the previous)

		unsigned char encodedDataCopy[CopySize];
		memcpy(encodedDataCopy, &mySource->getEncodedData()[currentByteLocation], CopySize);

		unsigned char encodedDataAnotherCopy[CopySize] = {0};

	    //Packet size adjustment
	    float actualPacketBytes = ceil(float(mySource->getEncodedDataSizes()[blockIndex])/
	    		                       float(BitsPerByte));

	    size_t roundedBytes = actualPacketBytes;

		memcpy(encodedDataAnotherCopy, mySource->getEncodedPacket(blockIndex), roundedBytes);
        shiftLeft(encodedDataAnotherCopy, (roundedBytes)*BitsPerByte, // get rid of the selection id
                  CodeOptionBitFieldFundamentalOrNoComp);



		size_t encodedLength(0);

		ushort splitValue[32]; // count to location of binary '1'
		int splitCount(1);     // count within byte
		int index(0);          // location in split array
		int copyIndex(0);      // current byte
		int shiftPosition(7);  // position within byte

		switch (selection)
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

				while (encodeCount < 32)
				{
					for(int bitIndex=(BitsPerByte-1); bitIndex>=0; bitIndex--)
					{

						if(encodeCount >= 32)
						{
							break;
						}

						encodedLength++;

						// Capture the encoded value
						//=====================================================
						// Count the bit if its '1'
						if ((encodedDataAnotherCopy[copyIndex] >> bitIndex) & 1)
						{
							encodeCount++;

							splitValue[index] = splitCount;

							#ifdef DEBUG
								//cout << "splitValue[" << index << "]=" << splitValue[index] << endl;
								//cout << "encodeCount=" << encodeCount << ", splitCount=" << splitCount << ", bitIndex=" << bitIndex << ", encodedLength=" << encodedLength << ", copyIndex=" << copyIndex << endl;
							#endif

							index++;
							splitCount = 0;
						}

						splitCount++;

					}

					copyIndex++;


				}

				encodedLength += CodeOptionBitFieldFundamentalOrNoComp;


				decodedSequence.decode(selection, splitValue, encodedDataAnotherCopy, blockIndex, residualsPtr);

				for(int index=0; index<32; index++)
				{
					if(referenceResiduals[blockIndex*32 + index] != residualsPtr[blockIndex*32 + index])
					{
						cout << "Mismatch residual value at Block:" << blockIndex << " Index:" << index << endl;
					}
				}

				// Total encoded length will be the current bit count, plus 32 x k-split
				encodedLength += (32 * (selection-1));

				break;

			case RiceAlgorithm::SecondExtensionOpt:
			case RiceAlgorithm::ZeroBlockOpt:

			    cout << "Unexpected code on count=" << count << endl;
			    exit(-1);
				break;


			case RiceAlgorithm::NoCompressionOpt:

				decodedNocomp.decode(selection, splitValue, encodedDataAnotherCopy, blockIndex, residualsPtr);

				for(int index=0; index<32; index++)
				{
					if(referenceResiduals[blockIndex*32 + index] != residualsPtr[blockIndex*32 + index])
					{
						cout << "Mismatch residual value at Block:" << blockIndex << " Index:" << index << endl;
					}
				}

				encodedLength = (32 * sizeof(ushort));
				encodedLength *= BitsPerByte;
				encodedLength += CodeOptionBitFieldFundamentalOrNoComp;

				break;

		}

	}


	// Perform unprediction of the residual values
	RiceAlgorithm::Predictor unprocessor(myHeader.xDimension, myHeader.yDimension, myHeader.zDimension);

	ushort* samples = new ushort[NumberOfSamples];
	unprocessor.getSamples(residualsPtr, samples);


	mySource->sendDecodedData(reinterpret_cast<char*>(samples), NumberOfSamples*sizeof(short));


    #ifdef DEBUG
    std::ofstream residualsStream;
    residualsStream.open("residualsGround.bin", ios::out | ios::in | ios::binary | ios::trunc);

    if (!residualsStream.is_open())
    {
        exit(EXIT_FAILURE);
    }
    //for(long index=0; index<NumberOfSamples; index++)
    for(long index=0; index<2000; index++)
    {
        cout << "residualsGround[" << index << "]=" << residualsPtr[index] << endl;
    }
    residualsStream.write(reinterpret_cast<char*>(residualsPtr), (1024*1024*6*2));
    residualsStream.close();
    #endif

    delete[] encodedBlockSizes;
    delete[] residualsPtr;
    delete[] samples;
}


void GroundSystem::readHeader()
{
    // Note: Header is not completely populated for all defined parameters.
    // Only what is applicable to the selected test raw data to
    // identify identical information.

    // Since the image written to the encoded file has already
    // been materialized, just read this directly rather than
    // re-reading from the file. Just trying to perform consecutive
    // encoding, decoding here
    unsigned char* encodedData = mySource->getEncodedData();

    memcpy(&myHeader.xDimension, &encodedData[1], sizeof(myHeader.xDimension));
    memcpy(&myHeader.yDimension, &encodedData[3], sizeof(myHeader.yDimension));
    memcpy(&myHeader.zDimension, &encodedData[5], sizeof(myHeader.zDimension));
    bigEndianVersusLittleEndian(myHeader.xDimension);
    bigEndianVersusLittleEndian(myHeader.yDimension);
    bigEndianVersusLittleEndian(myHeader.zDimension);
}

void GroundSystem::getExpectedNextPacketPosition(unsigned char* currentEncodingPtr, int packetBitLength, int &byte, int &bit, ulong count)
{
    static double currentTotalLength(19); // In bytes...skip the header

    double endLength = currentTotalLength; // In bytes
    int currentLocationByte = endLength;
    int currentLocationBit = (endLength - (long)endLength) * BitsPerByte;

    unsigned char selectionBytes[2]; // might span 2 bytes
    memcpy(selectionBytes, (currentEncodingPtr + currentLocationByte), 2);
    shiftLeft(selectionBytes, 16, currentLocationBit);
    selectionBytes[0] >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);

    RiceAlgorithm::CodingSelection selection;
    selection = CodingSelection(selectionBytes[0]);

    #ifdef DEBUG
    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
       ((count >= LowerRange2) && (count <= UpperRange2)))
            cout << "count=" << count << " Current ID:K" << int(selection-1);
    #endif

    currentTotalLength += (double(packetBitLength)/double(BitsPerByte));
    endLength = currentTotalLength;

    int nextLocationByte = endLength;
    int nextLocationBit = (endLength - (long)endLength) * BitsPerByte;

    memcpy(selectionBytes, (currentEncodingPtr + nextLocationByte), 2);
    shiftLeft(selectionBytes, 16, nextLocationBit);
    selectionBytes[0] >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);

    selection = CodingSelection(selectionBytes[0]);

    #ifdef DEBUG
    if(((count >= LowerRange1) && (count <= UpperRange1)) ||
       ((count >= LowerRange2) && (count <= UpperRange2)))
            cout << "...Next ID:K" << int(selection-1) << endl;
    #endif
}


