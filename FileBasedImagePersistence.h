/*
 * FileBasedImagePersistence.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#ifndef FILEBASEDIMAGEPERSISTENCE_H_
#define FILEBASEDIMAGEPERSISTENCE_H_

#include <ImagePersistence.h>
#include <fstream>
#include <sstream>

//:TODO: I see this as a very temporary persistence mechanism
// until I elect what to use (i.e. PFW, Boost Serialization library, etc.)
// Will be described architecturally, probably within a specific layer

namespace RiceAlgorithm
{

class FileBasedImagePersistence: public ImagePersistence
{
    public:
        FileBasedImagePersistence(const char* baseFilename, unsigned int x=0, unsigned int y=0, unsigned int z=0);
        virtual ~FileBasedImagePersistence();

        // The idea here is to write all encoded data at once to persistence
        // This operation in not part of the compression.
        virtual void writeEncodedData();

        virtual void readEncodedData(size_t bytesToRead);

        void writeDecodedData();

        const char* getBaseName() { return myBaseFileStream.str().c_str(); }


    protected:
        virtual void setSamples(uint scanNumber);

    private:
        std::ostringstream myBaseFileStream;

        std::ifstream mySampleStream;
        unsigned long myLength;

        // will need to both write to and read from this stream
        std::ofstream myEncodedStream;


        std::ofstream myDecodedStream;

        // Stitch together the separate encoded blocks for final
        // output to compressed file. Since application primarily
        // will work with the separate encoded blocks, this is
        // mainly for verification. File I/O times are not included
        // in the performance parameters.
        void stitchEncoded();
};

} /* namespace RiceAlgorithm */

#endif /* FILEBASEDIMAGEPERSISTENCE_H_ */
