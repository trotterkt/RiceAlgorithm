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
    unsigned int code_len = (unsigned int)-1;


    //*** TODO: Right now -- does not seem to be applicable to test image ***//

    return code_len;
}

} /* namespace RiceAlgorithm */
