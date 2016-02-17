/*
 * SplitSequence.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#ifndef SPLITSEQUENCE_H_
#define SPLITSEQUENCE_H_

#include <AdaptiveEntropyEncoder.h>

namespace RiceAlgorithm
{

class SplitSequence : public AdaptiveEntropyEncoder
{
	public:
		SplitSequence(size_t sampleBlockSize);
		virtual ~SplitSequence();

        unsigned int encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection);
};

} /* namespace RiceAlgorithm */

#endif /* SPLITSEQUENCE_H_ */
