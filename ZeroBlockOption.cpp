/*
 * ZeroBlockOption.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
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

unsigned int ZeroBlockOption::encode(boost::dynamic_bitset<> &encodedStream, CodingSelection &selection)
{
	selection = ZeroBlockOpt;
    unsigned int code_len = (unsigned int)-1;


    //*** TODO: Right now -- does not seem to be applicable to test image ***//

    return code_len;
}

} /* namespace RiceAlgorithm */
