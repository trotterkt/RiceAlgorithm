/*
 * AdaptiveEntropyEncoder.h
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#ifndef ADAPTIVEENTROPYENCODER_H_
#define ADAPTIVEENTROPYENCODER_H_

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

// Allow for easier cross compile on Windows
#ifdef __MINGW32__
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif

namespace RiceAlgorithm
{
const size_t BlockSize(32);

enum CodingSelection {K0=0,             // Fundamental Sequence (FS) is the same as Split-Sequence, with K=0
					  K1, K2, K3, K4, K5,
					  K6, K7, K8, K9, K10,
					  K11, K12, K13, K14,
					  SplitSeq, ZeroBlockOpt, SecondExtensionOpt, NoCompressionOpt};

//:TODO: will probably change this to be more in line with a Factory pattern

class AdaptiveEntropyEncoder
{

public:
	AdaptiveEntropyEncoder(size_t sampleBlockSize);
	virtual ~AdaptiveEntropyEncoder();
	AdaptiveEntropyEncoder(const AdaptiveEntropyEncoder &right);

	//:TODO: this shoud probabily be in constructor
	void setSamples(ushort* samples) { memcpy(myInputSamples, samples, myBlockSize); }

    // encoding in the base class is basically nothing
	virtual unsigned int encode(unsigned int* encodedBlock, CodingSelection &selection) {  };

protected:
	ushort* myInputSamples; // use input for final encoding as well, with sync frame
	size_t myBlockSize;      // -- and this will be used for the encoded size too
	static CodingSelection myCodingSelection;

};

} /* namespace RiceAlgorithm */

#endif /* ADAPTIVEENTROPYENCODER_H_ */
