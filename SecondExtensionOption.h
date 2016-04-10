/*
 * SecondExtensionOption.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
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
		void decode(CodingSelection selection, ushort* splitValue, ushort* encodedStream, ushort* preprocessedStream);

};

} /* namespace RiceAlgorithm */

#endif /* SECONDEXTENSIONOPTION_H_ */
