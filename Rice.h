/*
 * Rice.h
 *
 *  Created on: Jul 17, 2015
 *      Author: trotterkt
 */

#ifndef RICE_H_
#define RICE_H_

//:TODO:
// (1)In testing, string together a hundred or so scans of the same 3 LandSat
// raw image files to approximate an actual operation.

// (2) Image parameters expected will be the same which will simplify the implementation
// (3) European Space Agency appears to take about 2 seconds each for decompression and compression
// on the test image when running sequentially.

class Rice
{
public:
	Rice();
	virtual ~Rice();

	void compress();
	void decompress();

private:

	// Preprocessor
	void predictor(); // This might turn out to be an associated class
	void performMapping();
	void performDemapping();

};

#endif /* RICE_H_ */
