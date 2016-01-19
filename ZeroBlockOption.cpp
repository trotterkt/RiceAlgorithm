/*
 * ZeroBlockOption.cpp
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#include <ZeroBlockOption.h>

namespace RiceAlgorithm
{

ZeroBlockOption::ZeroBlockOption(size_t sampleBlockSize)
	: AdaptiveEntropyEncoder(sampleBlockSize)
{
	// TODO Auto-generated constructor stub

}

ZeroBlockOption::~ZeroBlockOption()
{
	// TODO Auto-generated destructor stub
}

unsigned int ZeroBlockOption::encode(unsigned int* encodedBlock, CodingSelection &selection)
{
	selection = ZeroBlockOpt;
}

} /* namespace RiceAlgorithm */
