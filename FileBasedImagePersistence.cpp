/*
 * FileBasedImagePersistence.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#include <FileBasedImagePersistence.h>
#include <AdaptiveEntropyEncoder.h>
#include <stdlib.h>
#include <ShiftFunctions.h>

using namespace std;

//:TODO: I see this as a very temporary persistence mechanism
// until I elect what to use (i.e. PFW, Boost Serialization library, etc.)
// Will be described architecturally, probably within a specific layer

namespace RiceAlgorithm
{

FileBasedImagePersistence::FileBasedImagePersistence(char* filename, unsigned int x, unsigned int y, unsigned int z) :
        ImagePersistence(x, y, z)
{
    // Filename ignores extension :TODO: will need to isolate persistence later
    myBaseFileStream << filename;

    mySampleStream.open((myBaseFileStream.str() + ".raw").c_str(), ios::in | ios::binary);

    if (!mySampleStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

    myEncodedStream.open((myBaseFileStream.str() + ".comp").c_str(), ios::out | ios::in | ios::binary | ios::trunc);

    if (!myEncodedStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

    myDecodedStream.open((myBaseFileStream.str() + ".raw_decoded").c_str(), ios::out | ios::binary | ios::trunc);

    if (!myDecodedStream.is_open())
    {
        exit(EXIT_FAILURE);
    }

}

FileBasedImagePersistence::~FileBasedImagePersistence()
{
    mySampleStream.close();
    myEncodedStream.close();
    myDecodedStream.close();
}

void FileBasedImagePersistence::writeEncodedData()
{
    // myEncoded data happens to currently have the header. Just write this
    // part. The remainder will be written by stitchEncoded()
    const int HeaderSize(19);
    myEncodedStream.write(reinterpret_cast<char*>(myEncodedData), HeaderSize);

    stitchEncoded();

}

void FileBasedImagePersistence::readEncodedData(size_t bytesToRead)
{
    myEncodedStream.write(reinterpret_cast<char*>(myEncodedData), myEncodedBytesWritten);
}

void FileBasedImagePersistence::writeDecodedData()
{
    // There is no header to the raw samples
    myDecodedStream.write(reinterpret_cast<char*>(myDecodedData), myDecodedBytesToWrite);
}

void FileBasedImagePersistence::setSamples(uint scanNumber)
{
    unsigned short int buffer(0);
    static unsigned long readElements(0);

    streamsize sampleSize = sizeof(buffer);

    uint blockSize = myXDimension * myYDimension * myZDimension;

    // Skip ahead on the next scan
    readElements = ((scanNumber-1) * blockSize);


    //Now, instead, we are in the situation that only the exact D bits are specified for
    //every sample.
    //I read two bytes (16 bits) at a time, eventually eliminating the most
    //significant bits, in case the length of the residuals is smaller than 16 bits,
    //keeping the remaining bits for the next residual
    //I repeat until the input file is empty
    //while ((readElements * sampleSize) < (myLength/scanNumber))
    while (readElements < (blockSize*scanNumber))
    {
        mySampleStream.read(reinterpret_cast<char*>(&buffer), sampleSize);

        // This assumes the data is in BSQ format and we do not need to adjust the indexing
        mySampleData[readElements] = buffer;

        readElements++;
    }

    // Determine if uneven number (1) of bytes left
    if ((myLength - mySampleStream.tellg()) == 1)
    {
        mySampleStream.read(reinterpret_cast<char*>(&buffer), 1);
        mySampleData[readElements] = buffer;
    }


    myCurrentScanNumber = scanNumber;
}

void FileBasedImagePersistence::stitchEncoded()
{
    vector<unsigned int> shiftRightList;
    vector<unsigned char> mergedLastFirstByteList;

    const int NumberOfEncodedBlocks((myXDimension * myYDimension * myZDimension)/32);

    int shiftRightValue(0);
    unsigned long totalEncodedLength(0);

    // gather the offsets between sequential blocks
    for(int index=0; index<NumberOfEncodedBlocks; index++)
    {
        totalEncodedLength += myEncodedSizes[index];

        // populate an array for the shift right values
        shiftRightValue =  totalEncodedLength % BitsPerByte;
        shiftRightList.push_back(shiftRightValue);


        // create these except for the last block
        if(index != (NumberOfEncodedBlocks-1))
        {
            unsigned char lastByte = (myEncodedDataList[index])[myEncodedSizes[index]/BitsPerByte];
            unsigned char firstByte = (myEncodedDataList[index+1])[0];

            if(shiftRightValue)
            {
                mergedLastFirstByteList.push_back(lastByte | (firstByte >> shiftRightValue));
            }
            else
            {
                mergedLastFirstByteList.push_back(lastByte);
            }
        }

    }

    // write the merged encoded blocks
    for(int index=0; index<NumberOfEncodedBlocks; index++)
    {
        const int EncodedBufferSize(sizeof(ushort)*32 + CodeOptionBitFieldFundamentalOrNoComp);
        char encodedBuffer[EncodedBufferSize] = {0};
        memcpy(encodedBuffer, myEncodedDataList[index], EncodedBufferSize);

        int encodedByteLength = myEncodedSizes[index]/BitsPerByte;

        if(index != 0 && shiftRightList[index-1])
        {
            shiftRight(reinterpret_cast<unsigned char*>(encodedBuffer), EncodedBufferSize, shiftRightList[index-1]);
            // first everything, but the first and last bytes
            myEncodedStream.write(reinterpret_cast<char*>(&encodedBuffer[1]), encodedByteLength-1);
        }
        else
        {
            // first everything, but the first and last bytes
            myEncodedStream.write(reinterpret_cast<char*>(encodedBuffer), encodedByteLength);
        }
        
        // then the merged byte
        myEncodedStream.put(mergedLastFirstByteList[index]);
    }
}

} /* namespace RiceAlgorithm */
