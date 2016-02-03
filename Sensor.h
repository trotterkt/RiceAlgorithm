/*
 * Sensor.h
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <Predictor.h>
#include <AdaptiveEntropyEncoder.h>
#include <SplitSequence.h>
#include <SecondExtensionOption.h>
#include <ZeroBlockOption.h>

const double LandsatDownlinkRate(384);

const ushort MaximumEncodedBlockSize(RiceAlgorithm::BlockSize);


inline static bool isSystemBigEndian()
{
	int endianCheck = 1;

	return ((*(reinterpret_cast<char*>(&endianCheck)) == 0));
}

template <typename T>
inline void bigEndianVersusLittleEndian(T &numberToTranslate)
{
	const size_t bytes = sizeof(T);

	if(!isSystemBigEndian())
	{
		uint8_t buffer[bytes];
		memcpy(buffer, &numberToTranslate, bytes);

		int i = 0;
		int j = bytes -1;

		while(i<j)
		{
			std::swap(buffer[i], buffer[j]);
			i++;
			j--;
		}
		memcpy(&numberToTranslate, buffer, bytes);
	}
}

class Sensor
{
	public:
		Sensor(char* filename, unsigned int x, unsigned int y, unsigned int z);
		virtual ~Sensor();
		ushort* getSamples(uint scanNumber=1);
		void process();

		 void operator=(RiceAlgorithm::AdaptiveEntropyEncoder& right){
			 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 memcpy(myEncodedBlock, right.getEncodedBlock(), right.getEncodedBlockSize());
			 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 myWinningEncodedLength = right.getEncodedBlockSize();
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 }
		 bool operator>(RiceAlgorithm::AdaptiveEntropyEncoder& right){  return false; }

	private:
		// prefix only :TODO: reassess for persistence
		std::ostringstream myFileStream;

	    ushort* mySamples;
	    unsigned int myEncodedBlock[MaximumEncodedBlockSize];

	    std::ifstream mySampleStream;
	    unsigned long myLength;

	    std::ofstream myEncodedStream;
	    void createHeader();

        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        RiceAlgorithm::Predictor myPreprocessor;
		//RiceAlgorithm::AdaptiveEntropyEncoder myEncoder;
		std::vector<class RiceAlgorithm::AdaptiveEntropyEncoder*> myEncoderList;

		unsigned int myWinningEncodedLength;

		//:TODO: These should instead be declared in the implementation file
		RiceAlgorithm::AdaptiveEntropyEncoder* noComp;
		RiceAlgorithm::SecondExtensionOption* secondExt;
		RiceAlgorithm::ZeroBlockOption* zeroBlock;
		RiceAlgorithm::SplitSequence* split; // this will become more specific

		void createEncodingCodes(RiceAlgorithm::AdaptiveEntropyEncoder& encoder);

};

#endif /* SENSOR_H_ */
