/*
 * Sensor.h
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdlib.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <Predictor.h>
#include <AdaptiveEntropyEncoder.h>
#include <SplitSequence.h>
#include <SecondExtensionOption.h>
#include <ZeroBlockOption.h>

const double LandsatDownlinkRate(384);

const ushort MaximumEncodedBlockSize(RiceAlgorithm::BlockSize + sizeof(char));

class Sensor
{
	public:
		Sensor(char* filename, unsigned int x, unsigned int y, unsigned int z);
		virtual ~Sensor();
		ushort* getSamples(uint scanNumber=1);
		void process();
		void getWinner(unsigned int* encodedBlock,
					   ushort codeLength,
				       RiceAlgorithm::CodingSelection selection,
				       bool lastType);

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

		//:TODO: These should instead be declared in the implementation file
		RiceAlgorithm::AdaptiveEntropyEncoder* noComp;
		RiceAlgorithm::SecondExtensionOption* secondExt;
		RiceAlgorithm::ZeroBlockOption* zeroBlock;
		RiceAlgorithm::SplitSequence* split;

};

#endif /* SENSOR_H_ */
