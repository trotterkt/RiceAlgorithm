/*
 * SplitSequence.h
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#ifndef SPLITSEQUENCE_H_
#define SPLITSEQUENCE_H_

#include <AdaptiveEntropyEncoder.h>

namespace RiceAlgorithm
{

class SplitSequence : public AdaptiveEntropyEncoder
{
	public:
		//SplitSequence(size_t sampleBlockSize, CodingSelection selection);
		SplitSequence(size_t sampleBlockSize);
		virtual ~SplitSequence();

		unsigned int encode(unsigned int* encodedBlock, CodingSelection &selection);
};

} /* namespace RiceAlgorithm */

#endif /* SPLITSEQUENCE_H_ */
