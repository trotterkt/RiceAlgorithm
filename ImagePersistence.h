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

        char* getEncodedData() { return myEncodedData; }

        virtual void sendEncodedData(char* encodedData) {
                                                            myEncodedData[myEncodedBytesWritten] = *encodedData;
                                                            myEncodedBytesWritten++;
                                                        }

        unsigned char getLastByte() { return myEncodedData[myEncodedBytesWritten-1]; }

        void setNextInsertionByte(unsigned long long byteIndex) { myEncodedBytesWritten = byteIndex; }

    protected:

        ushort* mySampleData;
        char* myEncodedData;

        virtual void setSamples(uint scanNumber) = 0;

        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        uint myCurrentScanNumber;

        unsigned long long myEncodedBytesWritten;


};

} /* namespace RiceAlgorithm */

#endif /* IMAGEPERSISTENCE_H_ */
