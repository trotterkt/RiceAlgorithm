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

		unsigned int encode(unsigned int* encodedBlock, CodingSelection &selection, char lastByte);

};

} /* namespace RiceAlgorithm */

#endif /* SECONDEXTENSIONOPTION_H_ */
