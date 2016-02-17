/*
 * ZeroBlockOption.h
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
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
