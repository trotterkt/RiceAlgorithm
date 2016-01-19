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
#include <Predictor.h>
#include <AdaptiveEntropyEncoder.h>
#include <SplitSequence.h>
#include <SecondExtensionOption.h>
#include <ZeroBlockOption.h>

const double LandsatDownlinkRate(384);

class Sensor
{
	public:
		Sensor(char* filename, unsigned int x, unsigned int y, unsigned int z);
		virtual ~Sensor();
		u_short* getSamples(uint scanNumber=1);
		void process();
		//std::vector<class AdaptiveEntropyEncoder> getEncoders() { return myEncoderList; }
		u_short* getWinner();

	private:
	    u_short* mySamples;
	    std::ifstream mySampleStream;
	    unsigned long myLength;


        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        RiceAlgorithm::Predictor myPreprocessor;
		//RiceAlgorithm::AdaptiveEntropyEncoder myEncoder;
		std::vector<class RiceAlgorithm::AdaptiveEntropyEncoder*> myEncoderList;

		RiceAlgorithm::AdaptiveEntropyEncoder* noComp;
		RiceAlgorithm::SecondExtensionOption* secondExt;
		RiceAlgorithm::ZeroBlockOption* zeroBlock;
		RiceAlgorithm::SplitSequence* split;
//		RiceAlgorithm::SplitSequence* split1;
//		RiceAlgorithm::SplitSequence* split2;
//		RiceAlgorithm::SplitSequence* split3;
//		RiceAlgorithm::SplitSequence* split4;
//		RiceAlgorithm::SplitSequence* split5;
//		RiceAlgorithm::SplitSequence* split6 ;
//		RiceAlgorithm::SplitSequence* split7;
//		RiceAlgorithm::SplitSequence* split8 ;
//		RiceAlgorithm::SplitSequence* split9;
//		RiceAlgorithm::SplitSequence* split10;
//		RiceAlgorithm::SplitSequence* split11;
//		RiceAlgorithm::SplitSequence* split12;
//		RiceAlgorithm::SplitSequence* split13;


};

#endif /* SENSOR_H_ */
