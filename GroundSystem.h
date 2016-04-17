/*
 * GroundSystem.h
 *
 *  Created on: Mar 28, 2016
 *      Author: trotterkt
 */

#ifndef GROUNDSYSTEM_H_
#define GROUNDSYSTEM_H_

#include <ImagePersistence.h>

namespace RiceAlgorithm
{

class GroundSystem
{
	public:
		GroundSystem(ImagePersistence* image);
		virtual ~GroundSystem();

		void process();

		// For validation
		ushort* getSamples()
		{
			return myRawSamples;
		}

	private:
		void readHeader();
		CompressedHeader myHeader;

		RiceAlgorithm::ImagePersistence* mySource;

		ushort* myRawSamples;

};

} /* namespace RiceAlgorithm */

#endif /* GROUNDSYSTEM_H_ */
