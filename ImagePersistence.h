/*
 * ImagePersistence.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#ifndef IMAGEPERSISTENCE_H_
#define IMAGEPERSISTENCE_H_

#include <sys/types.h>
#include <string.h>

// Note that if member types are not defined as being of similar size
// there can be an alignment problem. See Annotated  C++ Ref Manual,
// Sec 5.3.2. This will not be an issue for this structure, since
// I will not be writing in out directly as a whole.

struct CompressedHeader
{
    char userData;
    short xDimension;
    short yDimension;
    short zDimension;

    char signSampDynRangeBsq1;   // sample type, reserved, dyn range, bsq(1)
    short bsq;
    short wordSizEncodeMethod; // reserved, out word size, encoding method,
                                 // reserved
    char predictBandMode;        // user input predictor band,
                                 // predictor full, reserved,
    char neighborRegSize;        // neighbor sum,
                                 //reserve, register size
    char predictWeightResInit;   // weight resolution, weight interval, initial weight,
                                 // final weight, reserved, initial weight table,
                                  // weight init resolution
    char predictWeightInitFinal;    // reserved, block size flag, restricted, ref interval
    char predictWeightTable;
    short blockSizeRefInterval;
};

//:TODO: I see this as a very temporary persistence mechanism
// until I elect what to use (i.e. PFW, Boost Serialization library, etc.)
// Will be described architecturally, probably within a specific layer

namespace RiceAlgorithm
{

class ImagePersistence
{
    public:
        ImagePersistence(unsigned int x, unsigned int y, unsigned int z);
        virtual ~ImagePersistence();

        ushort* getSampleData(uint scanNumber) {
                                                  setSamples(scanNumber);
                                                  return mySampleData;
                                               }

        unsigned char* getEncodedData() { return myEncodedData; }

        void setEncodedByteLocation(long byteLocation) { myEncodedBytesWritten = byteLocation; }

        virtual void sendEncodedData(char* encodedData) {
                                                            myEncodedData[myEncodedBytesWritten] = *encodedData;
                                                            myEncodedBytesWritten++;
                                                        }

        virtual void sendDecodedData(char* decodedData, const long long imageSize) {
                                                                                       memcpy(myDecodedData, decodedData, imageSize);
                                                                                       myDecodedBytesToWrite = imageSize;
                                                                                   }

        ushort* getDecodedData() { return reinterpret_cast<ushort*>(myDecodedData); }

        unsigned char getLastByte() { return myEncodedData[myEncodedBytesWritten-1]; }

        void setNextInsertionByte(unsigned long long byteIndex) { myEncodedBytesWritten = byteIndex; }

        unsigned long long getBytesWritten() { return myEncodedBytesWritten; }

    protected:

        ushort* mySampleData;
        unsigned char* myEncodedData;
        unsigned char* myDecodedData;

        virtual void setSamples(uint scanNumber) = 0;

        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        uint myCurrentScanNumber;

        long myEncodedBytesWritten;
        long myDecodedBytesToWrite;
};

} /* namespace RiceAlgorithm */

#endif /* IMAGEPERSISTENCE_H_ */
