/*
 * ZeroBlockOption.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#ifndef ZEROBLOCKOPTION_H_
#define ZEROBLOCKOPTION_H_

#include <AdaptiveEntropyEncoder.h>

namespace RiceAlgorithm
{

class ZeroBlockOption : public AdaptiveEntropyEncoder
{
	public:
		ZeroBlockOption(size_t sampleBlockSize);
		virtual ~ZeroBlockOption();

	    unsigned int encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection);
};

} /* namespace RiceAlgorithm */

#endif /* ZEROBLOCKOPTION_H_ */
