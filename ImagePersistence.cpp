/*
 * ImagePersistence.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#include <ImagePersistence.h>

//:TODO: I see this as a very temporary persistence mechanism
// until I elect what to use (i.e. PFW, Boost Serialization library, etc.)
// Will be described architecturally, probably within a specific layer

namespace RiceAlgorithm
{

ImagePersistence::ImagePersistence(unsigned int x, unsigned int y, unsigned int z) :
		mySampleData(0), myEncodedData(0), myEncodedBytesWritten(0), myXDimension(x), myYDimension(y), myZDimension(z)
{
	mySampleData =
			reinterpret_cast<ushort*>(new ushort[myXDimension * myYDimension * myZDimension]);

	// Encoded data will never be more than the original, and likely shorter
	myEncodedData = reinterpret_cast<unsigned char*>(new ushort[myXDimension * myYDimension
			* myZDimension]);

	myDecodedData = reinterpret_cast<unsigned char*>(new ushort[myXDimension * myYDimension
			* myZDimension]);

	myEncodedSizes = new ushort[(myXDimension * myYDimension * myZDimension)/32];
}

ImagePersistence::~ImagePersistence()
{
	delete[] mySampleData;

	delete[] myEncodedData;

	delete[] myDecodedData;

	delete[] myEncodedSizes;
}

} /* namespace RiceAlgorithm */
