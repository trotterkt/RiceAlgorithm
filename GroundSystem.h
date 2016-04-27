/*
 * GroundSystem.h
 *
 *  Created on: Mar 28, 2016
 *      Author: trotterkt
 */

#ifndef GROUNDSYSTEM_H_
#define GROUNDSYSTEM_H_

#include <ImagePersistence.h>
#include <ShiftFunctions.h>
#include <AdaptiveEntropyEncoder.h>

namespace RiceAlgorithm
{

class GroundSystem
{
	public:
		GroundSystem(ImagePersistence* image);
		virtual ~GroundSystem();

		void process();

		// For validation
		ushort* getSamples(){ return myRawSamples; }

		ushort* getResiduals() { return myResidualsPtr; }


	private:
		void readHeader();
		CompressedHeader myHeader;

		RiceAlgorithm::ImagePersistence* mySource;

		ushort* myRawSamples;
		ushort* myResidualsPtr;
		
		//:KLUDGE: method
	    // This is very much a kludge - since I only expect to see non-zero
	    // selection id's, look for the first non-zero data. I'll use this to determine
	    // where to shift left. If this ever turned into production code it should be fixed
        //*********************************************************************************
		void adjustPackeDataPosition(unsigned char* packedData, size_t dataBitLength)
		{
			int shiftIndex=0;
			for(shiftIndex; shiftIndex<7; shiftIndex++) // expect it to be fairly soon
			{
				if(packedData[shiftIndex])
				{
					break;
				}
			}
			shiftIndex *= RiceAlgorithm::BitsPerByte;
			shiftLeft(packedData, dataBitLength, shiftIndex);
		}

        void getExpectedNextPacketPosition(unsigned char* currentEncodingPtr, int packetBitLength, int &byte, int &bit, ulong count);

};

} /* namespace RiceAlgorithm */

#endif /* GROUNDSYSTEM_H_ */
