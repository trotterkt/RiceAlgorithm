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

		unsigned int encode(unsigned int* encodedBlock, boost::dynamic_bitset<> &encodedStream, CodingSelection &selection, char lastByte);
};

} /* namespace RiceAlgorithm */

#endif /* SPLITSEQUENCE_H_ */
