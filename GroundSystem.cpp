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
    }
}

void GroundSystem::process()
{
	// :TODO: This begins the decoding
	readHeader();

	// Having the raw sample dimensions from the header, allocate space for
	// the decoding
	const long NumberOfSamples(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension);

	myRawSamples = new ushort[NumberOfSamples];

	// Encoded data should begin right after the header (byte 19)

	// 1st grab the Encoded ID
	const ulong HeaderLength(19u);
	ulong totalEncodedLength(HeaderLength * BitsPerByte);

	unsigned int additionalBits(0);

	ushort* encodedBlockSizes = new ushort[(myHeader.xDimension *
	                                        myHeader.yDimension *
			                                myHeader.zDimension) / 32];
	ulong count(0);

	SplitSequence decodedSequence(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension);

	//:TODO: This might better belong in an object that reverses the prediction
	ushort* residualsPtr = reinterpret_cast<ushort*>(new ushort[myHeader.xDimension * myHeader.yDimension * myHeader.zDimension]);

	// Read in one 32-sample block at a time (not on byte boundary)
	const long MaximumBlocks(NumberOfSamples / 32);
    ulong currentByteLocation = totalEncodedLength / BitsPerByte;

	for (long blockIndex = 0; blockIndex < MaximumBlocks; blockIndex++)
	{
        count++;

	    //:KLUDGE: At least on Windows...currentByteLocation is being
	    // corrupted somehow. So using an identical variable
	    // currentByteLocation2
	    //=============================================================
        currentByteLocation = totalEncodedLength / BitsPerByte;
        ulong currentByteLocation2 = totalEncodedLength / BitsPerByte;
        //=============================================================

		// Account for selection value not being on a byte boundary
		// The selection ID may span as much as two bytes
		unsigned char selectionBytes[2] = {0};
		memcpy(selectionBytes, &mySource->getEncodedData()[currentByteLocation2], 2);
		shiftLeft(selectionBytes, 16, additionalBits);

		selectionBytes[0] >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);
		selectionBytes[0] = selectionBytes[0] & 0xf;


		CodingSelection selection = CodingSelection(selectionBytes[0]);


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
        const unsigned int CopySize(32 * sizeof(ushort) + 1); // Encoded data will be no larger than this
		unsigned char encodedDataCopy[CopySize];
		memcpy(encodedDataCopy, &mySource->getEncodedData()[currentByteLocation2], CopySize);

		#ifdef DEBUG

		//*******************************************
        if(((count >= LowerRange1) && (count <= UpperRange1)) ||
           ((count >= LowerRange2) && (count <= UpperRange2)))
		{
			cout << "winning encoding          ==>";

			for(int countIndex=0; countIndex < CopySize; countIndex++)
			{
				cout << hex << int(encodedDataCopy[countIndex]) << " ";
			}
			cout << dec << endl;
		}
		//*******************************************

		#endif

        shiftLeft(encodedDataCopy, CopySize * BitsPerByte + CodeOptionBitFieldFundamentalOrNoComp,
                  (CodeOptionBitFieldFundamentalOrNoComp + additionalBits));

		#ifdef DEBUG
		//*******************************************
        if(((count >= LowerRange1) && (count <= UpperRange1)) ||
           ((count >= LowerRange2) && (count <= UpperRange2)))
		{
			cout << "winning encoding (shifted)==>";

			for(int countIndex=0; countIndex < CopySize; countIndex++)
			{
				cout << hex << int(encodedDataCopy[countIndex]) << " ";
			}
			cout << dec << endl;
		}
		//*******************************************
		#endif

		size_t encodedLength(0);

		ushort splitValue[32];
		int splitCount(0);
		int index(0);
		int copyIndex(0);

		int shiftPosition(7);

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
					// Count the bit if its '1'
					encodeCount += ((encodedDataCopy[copyIndex] >> (shiftPosition)) & 1);

					splitCount++;

					// Capture the encoded value
					//=====================================================

					if ((encodedDataCopy[copyIndex] >> (shiftPosition)) & 1)
					{
						splitValue[index] = splitCount;

						#ifdef DEBUG
			            if(((count >= LowerRange1) && (count <= UpperRange1)) ||
			               ((count >= LowerRange2) && (count <= UpperRange2)))
				                cout << "\nencodedSizeList[" << index << "]=" << splitValue[index] << endl;
						#endif

						index++;
						splitCount = 0;
					}

					shiftPosition--;
					encodedLength++;

					if (shiftPosition < 0)
					{
						copyIndex++;
						shiftPosition = BitsPerByte - 1;
					}
				}

				encodedLength += CodeOptionBitFieldFundamentalOrNoComp;


				decodedSequence.decode(selection, splitValue, encodedDataCopy, blockIndex, residualsPtr);

				// Total encoded length will be the current bit count, plus 32 x k-split
				encodedLength += (32 * (selection-1));
				break;

			case RiceAlgorithm::SecondExtensionOpt:
			case RiceAlgorithm::ZeroBlockOpt:

			    cout << "Unexpected code" << endl;
			    exit(-1);
				break;


			case RiceAlgorithm::NoCompressionOpt:
				encodedLength = (32 * sizeof(ushort));
				encodedLength *= BitsPerByte;
				encodedLength += CodeOptionBitFieldFundamentalOrNoComp;

				memcpy(&residualsPtr[blockIndex*32], encodedDataCopy, sizeof(ushort)*32);

				break;


		}

		totalEncodedLength += encodedLength;

		//currentByteLocation = (totalEncodedLength / BitsPerByte);

		#ifdef DEBUG
        if(((count >= LowerRange1) && (count <= UpperRange1)) ||
           ((count >= LowerRange2) && (count <= UpperRange2)))
				cout << "currentByteLocation=" << currentByteLocation << ", totalEncodedLength="
					 << totalEncodedLength << endl;
		#endif

		additionalBits = totalEncodedLength % BitsPerByte;

		#ifdef DEBUG
        if(((count >= LowerRange1) && (count <= UpperRange1)) ||
           ((count >= LowerRange2) && (count <= UpperRange2)))
				cout << "additional bits=" << additionalBits << endl;
		#endif
        
	}


	// Perform unprediction of the residual values
	RiceAlgorithm::Predictor unprocessor(myHeader.xDimension, myHeader.yDimension, myHeader.zDimension);

	ushort* samples = new ushort[NumberOfSamples];
	unprocessor.getSamples(residualsPtr, samples);


	mySource->sendDecodedData(reinterpret_cast<char*>(samples), NumberOfSamples*sizeof(short));


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
