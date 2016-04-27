/*
 * Sensor.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <sstream>
#include <Predictor.h>
#include <AdaptiveEntropyEncoder.h>
#include <SplitSequence.h>
#include <SecondExtensionOption.h>
#include <ZeroBlockOption.h>
#include <ImagePersistence.h>
#include <boost/dynamic_bitset.hpp>
#include <ShiftFunctions.h>
#include <GroundSystem.h>

const double LandsatDownlinkRate(384);

const ushort MaximumEncodedBlockSize(RiceAlgorithm::BlockSize);
const ulong HeaderLength(19u);

class Sensor
{
	public:
		Sensor(RiceAlgorithm::ImagePersistence* image, unsigned int x, unsigned int y,
				unsigned int z);
		virtual ~Sensor();

		void process();

		void operator=(RiceAlgorithm::AdaptiveEntropyEncoder& right)
		{
			memcpy(myEncodedBlock, right.getEncodedBlock(), right.getEncodedBlockSize());
			myWinningEncodedLength = right.getEncodedBlockSize();
		}
		bool operator>(RiceAlgorithm::AdaptiveEntropyEncoder& right)
		{
			return false;
		}

		void setImageSource(RiceAlgorithm::ImagePersistence* image)
		{
			mySource = image;
		}

		RiceAlgorithm::GroundSystem* getGround()
		{
			return groundPtr;
		}

		// For validation
		ushort* getSamples() { return mySamples; }

	private:
		RiceAlgorithm::ImagePersistence* mySource;

		ushort* mySamples;
		unsigned int myEncodedBlock[MaximumEncodedBlockSize];

		// append all encoded blocks before sending at once
		boost::dynamic_bitset<> myFullEncodedStream;

		void sendHeader();

		unsigned int myXDimension;
		unsigned int myYDimension;
		unsigned int myZDimension;

		RiceAlgorithm::Predictor myPreprocessor;
		std::vector<class RiceAlgorithm::AdaptiveEntropyEncoder*> myEncoderList;

		unsigned int myWinningEncodedLength;

		//:TODO: These should instead be declared in the implementation file
		RiceAlgorithm::AdaptiveEntropyEncoder* noComp;
		RiceAlgorithm::SecondExtensionOption* secondExt;
		RiceAlgorithm::ZeroBlockOption* zeroBlock;
		RiceAlgorithm::SplitSequence* split; // this will become more specific

		void sendEncodedSamples(boost::dynamic_bitset<> &encodedStream, unsigned int encodedLength = 0);

		template<typename T> void packCompressedData(T data,
				boost::dynamic_bitset<unsigned char> &packedData,
				ulong bitSize = sizeof(T) * RiceAlgorithm::BitsPerByte)
		{

			size_t numberOfBytes = sizeof(data);

			// whatever type it is, see it as a collection of bytes
			char* ptrData = reinterpret_cast<char*>(&data);

			size_t currentSize = packedData.size();

			// Since the data is of uneven types, all are read in as single bytes
			for (int index = 0; index < numberOfBytes; index++)
			{
				packedData.append(ptrData[index]);
			}

			packedData.resize(currentSize + bitSize);

		}

		ulong writeCompressedData(boost::dynamic_bitset<unsigned char> &packedData, size_t bitSize =
				0, bool flag = false);

		bool getLastByte(unsigned char *lastByte);

		void getExpectedNextPacketPosition(unsigned char* currentEncodingPtr, int packetBitLength, int &byte, int &bit, ulong count);

		unsigned int myEncodedBitCount;

		RiceAlgorithm::GroundSystem* groundPtr;

		//:KLUDGE: method
	    // This is very much a kludge - since I only expect to see non-zero
	    // selection id's, look for the first non-zero data. I'll use this to determine
	    // where to shift left. If this ever turned into production code it should be fixed
        //*********************************************************************************
		void adjustPackeDataPosition(unsigned char* packedData, size_t dataBitLength)
		{
			int shiftIndex=0;
			for(shiftIndex; shiftIndex<10; shiftIndex++) // expect it to be fairly soon
			{
				if(packedData[shiftIndex])
				{
					break;
				}
			}
			shiftIndex *= RiceAlgorithm::BitsPerByte;
			shiftLeft(packedData, dataBitLength, shiftIndex);

			// Look again within a byte, since I currently don't expect
			// selection values less than K7 (binary 1000)
			// So, shift left until I get non-zero bit
			while(!(packedData[0] & 0x80))
			{
				shiftLeft(packedData, dataBitLength, 1);
			}
		}

};

#endif /* SENSOR_H_ */
