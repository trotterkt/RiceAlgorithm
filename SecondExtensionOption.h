/*
 * SecondExtensionOption.h
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#ifndef SECONDEXTENSIONOPTION_H_
#define SECONDEXTENSIONOPTION_H_

#include <AdaptiveEntropyEncoder.h>

namespace RiceAlgorithm
{

class SecondExtensionOption : public AdaptiveEntropyEncoder
{
	public:
		SecondExtensionOption(size_t sampleBlockSize);
		virtual ~SecondExtensionOption();

		unsigned int encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection);

};

} /* namespace RiceAlgorithm */

#endif /* SECONDEXTENSIONOPTION_H_ */
