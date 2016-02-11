/*
 * Sensor.h
 *
 *  Created on: Jan 15, 2016
 *      Author: trotterkt
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <Predictor.h>
#include <AdaptiveEntropyEncoder.h>
#include <SplitSequence.h>
#include <SecondExtensionOption.h>
#include <ZeroBlockOption.h>
#include <boost/dynamic_bitset.hpp>

const double LandsatDownlinkRate(384);

const ushort MaximumEncodedBlockSize(RiceAlgorithm::BlockSize);
const ushort BitsPerByte(8);

inline static bool isSystemBigEndian()
{
	int endianCheck = 1;

	return ((*(reinterpret_cast<char*>(&endianCheck)) == 0));
}

template <typename T>
inline void bigEndianVersusLittleEndian(T &numberToTranslate)
{
	const size_t bytes = sizeof(T);

	if(!isSystemBigEndian())
	{
		uint8_t buffer[bytes];
		memcpy(buffer, &numberToTranslate, bytes);

		int i = 0;
		int j = bytes -1;

		while(i<j)
		{
			std::swap(buffer[i], buffer[j]);
			i++;
			j--;
		}
		memcpy(&numberToTranslate, buffer, bytes);
	}
}

// Note that if member types are not defined as being of similar size
// there can be an alignment problem. See Annotated  C++ Ref Manual,
// Sec 5.3.2
//struct CompressedHeader
//{
//    char userData;
//    char xDimension[2];
//    char yDimension[2];
//    char zDimension[2];
//
//    char signSampDynRangeBsq1;   // sample type, reserved, dyn range, bsq(1)
//    char bsq[2];
//    char wordSizEncodeMethod[2]; // reserved, out word size, encoding method,
//                                 // reserved
//    char predictBandMode;        // user input predictor band,
//                                 // predictor full, reserved,
//    char neighborRegSize;        // neighbor sum,
//                                 //reserve, register size
//    char predictWeightResInit;   // weight resolution, weight interval, initial weight,
//                                 // final weight, reserved, initial weight table,
//                                  // weight init resolution
//    char predictWeightInitFinal;    // reserved, block size flag, restricted, ref interval
//    char predictWeightTable;
//    char blockSizeRefInterval[2];
//};

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

class Sensor
{
	public:
		Sensor(char* filename, unsigned int x, unsigned int y, unsigned int z);
		virtual ~Sensor();
		ushort* getSamples(uint scanNumber=1);
		void process();

		 void operator=(RiceAlgorithm::AdaptiveEntropyEncoder& right){
			 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 memcpy(myEncodedBlock, right.getEncodedBlock(), right.getEncodedBlockSize());
			 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 myWinningEncodedLength = right.getEncodedBlockSize();
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 }
		 bool operator>(RiceAlgorithm::AdaptiveEntropyEncoder& right){  return false; }

	private:
		// prefix only :TODO: reassess for persistence
		std::ostringstream myFileStream;

	    ushort* mySamples;
	    unsigned int myEncodedBlock[MaximumEncodedBlockSize];

	    std::ifstream mySampleStream;
	    unsigned long myLength;

	    // will need to both write to and read from this stream
	    std::fstream myEncodedStream;
	    void createHeader();

        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        RiceAlgorithm::Predictor myPreprocessor;
		//RiceAlgorithm::AdaptiveEntropyEncoder myEncoder;
		std::vector<class RiceAlgorithm::AdaptiveEntropyEncoder*> myEncoderList;

		unsigned int myWinningEncodedLength;

		//:TODO: These should instead be declared in the implementation file
		RiceAlgorithm::AdaptiveEntropyEncoder* noComp;
		RiceAlgorithm::SecondExtensionOption* secondExt;
		RiceAlgorithm::ZeroBlockOption* zeroBlock;
		RiceAlgorithm::SplitSequence* split; // this will become more specific

		void createEncodingCodes(RiceAlgorithm::AdaptiveEntropyEncoder& encoder);

		// Templated method to write out information to compressed file. This is
		// necessary in writing information which is likely not to exist on a
		// byte boundary.
        static ulong bytesWritten; // new file pointer should be at this location
        static ulong bitsWritten;

		template<typename T> void packCompressedData(T data, boost::dynamic_bitset<unsigned char> &filter, ulong bitSize=sizeof(T)*BitsPerByte)
        {

		    size_t numberOfBytes = sizeof(data);

		    // whatever type it is, see it as a collection of bytes
		    char* ptrData = reinterpret_cast<char*>(&data);

		    // Since the data is of uneven types, all are read in as single bytes
		    for(int index=0; index<numberOfBytes; index++)
		    {
		        filter.append(ptrData[index]);
		    }

//            boost::dynamic_bitset<> testBitset(128, 0xffff);
//            boost::dynamic_bitset<> testBitset2(128, 0xffff);
//            testBitset << sizeof(ulong)*BitsPerByte;
//            testBitset |= testBitset2;
//
//            size_t blocks = testBitset.num_blocks();
//
//		    size_t dataSize = sizeof(T)*BitsPerByte;
//
//		    // Tack the next piece of data at the end of the list bit
//		    // so I need to Or it
//		    char currentData(0);
//		    long currentPosition = myEncodedStream.tellp();
//
//		    if(currentPosition > 0)
//		    {
//		        myEncodedStream.seekp(currentPosition - 1);
//		        myEncodedStream.read(&currentData, 1);
//		    }
//
//
//		    // combine the existing data with the new
//            boost::dynamic_bitset<> pendingData(dataSize+BitsPerByte, currentData);
//            boost::dynamic_bitset<> newData(dataSize+BitsPerByte, data);
//            //ulong pendingData(currentData);
//            //ulong newData(data);
//		    pendingData <<= dataSize;
//		    pendingData |= newData;
//
//		    ulong newValue = pendingData.to_ulong();
//
//            myEncodedStream.write(reinterpret_cast<char*>(&newData), dataSize/BitsPerByte);
//            //myEncodedStream.write(reinterpret_cast<char*>(&pendingData), dataSize/BitsPerByte);
//
		    // figure out the impending written size
		    bitsWritten += bitSize;
		    if(bitsWritten >= BitsPerByte)
		    {
		        size_t numberEquivBytes = bitsWritten/BitsPerByte;
		        bytesWritten += numberEquivBytes;
		        bitsWritten -= (numberEquivBytes*BitsPerByte);
		    }
//
//
//
        }

        void writeCompressedData(boost::dynamic_bitset<unsigned char> &filter);
};

#endif /* SENSOR_H_ */
