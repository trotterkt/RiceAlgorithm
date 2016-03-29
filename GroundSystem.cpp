/*
 * GroundSystem.cpp
 *
 *  Created on: Mar 28, 2016
 *      Author: trotterkt
 */

#include <GroundSystem.h>
#include <Sensor.h>
#include <Endian.h>

namespace RiceAlgorithm
{

GroundSystem::GroundSystem(ImagePersistence* image) : mySource(image), myRawSamples(0)
{
    memset(&myHeader, 0, sizeof(CompressedHeader));
}

GroundSystem::~GroundSystem()
{
    if(myRawSamples)
    {
        delete [] myRawSamples;
    }
}

void GroundSystem::process()
{
   // :TODO: This begins the decoding
    readHeader(); 

    // Having the raw sample dimensions from the header, allocate space for
    // the decoding
    myRawSamples = new ushort[myHeader.xDimension * myHeader.yDimension * myHeader.zDimension];


    // Encoded data should begin right after the header (byte 19)

}


void GroundSystem::readHeader()
{
    // Note: Header is not completely populated for all defined parameters.
    // Only what is applicable to the selected test raw data to
    // identify identical information.

    // Since the image written to the encoded file has already
    // been materialized, just read this directly rather than
    // re-reading from the file. Just trying to perform consecutive
    // encoding, decoding here
    char* encodedData = mySource->getEncodedData();

    memcpy(&myHeader.xDimension, &encodedData[1], sizeof(myHeader.xDimension));
    memcpy(&myHeader.yDimension, &encodedData[3], sizeof(myHeader.yDimension));
    memcpy(&myHeader.zDimension, &encodedData[5], sizeof(myHeader.zDimension));
    bigEndianVersusLittleEndian(myHeader.xDimension);
    bigEndianVersusLittleEndian(myHeader.yDimension);
    bigEndianVersusLittleEndian(myHeader.zDimension);

}

} /* namespace RiceAlgorithm */
