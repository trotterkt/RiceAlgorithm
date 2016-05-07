/*
 * rice_main.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#include <iostream>
#include <Sensor.h>
#include <Predictor.h>
#include <Timing.h>
#include <FileBasedImagePersistence.h>
#include <DebuggingParameters.h>


    
using namespace RiceAlgorithm;
using namespace std;

static unsigned char watchEncoded1(0);
static unsigned char watchEncoded2(0);

int main(int argc, char *argv[])
{

    if (argc < 2)
    {   // Expect 2 arguments: the program name,
	    // and the raw image file for compression
		std::cerr << "\nUsage: " << argv[0] << "\nProvide basename for raw data to compress and subsequently decompress." << std::endl;
		return 1;
    }


    std::string baseImageFilename = argv[1];


    cout << "\n\n";
    cout << "*********************************************************************" << endl;
    cout << "*                                                                   *" << endl;
    cout << "*                         RICE ALGORITHIM                           *" << endl;
    cout << "*                   Serialized Implementation                       *" << endl;
    cout << "*                                                                   *" << endl;
    cout << "*          CSU Fullerton, MSE Graduate Project, Fall 2016           *" << endl;
    cout << "*                          Keir Trotter                             *" << endl;
    cout << "*                                                                   *" << endl;
    cout << "*                                                                   *" << endl;
    cout << "*********************************************************************\n\n\n" << endl;

    // These parameters are what is utilized for LandSat
    const int Rows(1024);
    const int Columns(1024);
    const int Bands(6);

    cout.precision(4);

    //******************************************************************************
    // Rice Compression Processing
    //******************************************************************************
   
    FileBasedImagePersistence image(baseImageFilename.c_str(), Rows, Columns, Bands);

	#ifdef DEBUG
		// Watchpoint Debugging
		//===============================================
		watchEncoded1 = image.getEncodedData()[700];
		watchEncoded2 = image.getEncodedData()[750];
		//===============================================
	#endif

    // Construct my LandSat sensor, which performs the compression of the supplied
    // raw image data per the Rice algorithm
	Sensor landsat(&image, Rows, Columns, Bands);

    timestamp_t t0 = getTimestamp();

    // Initiate the Rice algorithm compression
	landsat.process();

    timestamp_t t1 = getTimestamp();
    //******************************************************************************


    // Write out the encoded data. This is outside of the compression processing
    image.writeEncodedData();



    //******************************************************************************
    // Rice Decompression Processing
    //******************************************************************************
    cout << "\n\nDecompressing Landsat_agriculture-u16be-6x1024x1024...\n" << endl;

    timestamp_t t2 = getTimestamp();


    // Kick off the associated decompression
    // The debug version launch is so that I can compare the
    // pre-processed residuals to that extracted from
    // decompression. These must match in able to have an
    // accurate decoding.
	#ifdef DEBUG
    	ushort* sensorResidualCompare(0);
    	sensorResidualCompare = landsat.getResiduals();
        landsat.getGround()->process(sensorResidualCompare);
	#else
    	landsat.getGround()->process();
	#endif

    timestamp_t t3 = getTimestamp();
    //******************************************************************************

    cout << "=============================================================" << endl;
    cout << "Total Rice Compression processing time   ==> " << fixed << getSecondsDiff(t0, t1) << " seconds"<< endl;
    cout << "Total Rice Decompression processing time ==> " << fixed << getSecondsDiff(t2, t3) << " seconds"<< endl;
    cout << "Total Round Trip ==> " << fixed << (getSecondsDiff(t0, t1) + getSecondsDiff(t2, t3))  << " seconds"<< endl;
    cout << "=============================================================" << endl;

    // Write out the decoded data. This is outside of the decompression processing
    image.writeDecodedData();



	#ifdef DEBUG

    // Check what I have
    //===============================================================================
    const int arraySize = (Rows*Columns*Bands);
    ushort* sensorResiduals = new ushort[arraySize];
    ushort* groundResiduals = new ushort[arraySize];

    ifstream sensorResidualsStream;
    sensorResidualsStream.open("residualsSensor.bin", std::ifstream::binary);
    sensorResidualsStream.read(reinterpret_cast<char*>(sensorResiduals), arraySize*2);

    ifstream groundResidualsStream;
    groundResidualsStream.open("residualsGround.bin", std::ifstream::binary);
    groundResidualsStream.read(reinterpret_cast<char*>(groundResiduals), arraySize*2);


    sensorResidualsStream.close();
    groundResidualsStream.close();


    #define matrixBsqIndexCheck(matrix, x, y, z) matrix[Rows*((z)*Columns + (y)) + (x)]
    int count = 0;
    for(int x=0; x<Rows; x++)
    {
        for(int y=0; y<Columns; y++)
        {
            for(int z=0; z<Bands; z++)
            {
                int index = (Rows*((z)*Columns + (y)) + (x));

                if(index >= 0 && index <= 6291456)
                {
                    ushort inSample = matrixBsqIndexCheck(landsat.getSamples(), x, y, z);
                    ushort outSample = matrixBsqIndexCheck(image.getDecodedData(), x, y, z);

                    ushort inResidual = matrixBsqIndexCheck(sensorResiduals, x, y, z);
                    ushort outResidual = matrixBsqIndexCheck(groundResiduals, x, y, z);


                    count++;

                    if (inResidual != outResidual)
                    {
                        cout << "Mismatch at Index:[" << index << "] x=" << x << ", y=" << y << ", z=" << z << " --Delta=" << (outSample - inSample);
                        cout << "...In_Sample=" << inSample << " Out_Sample=" << outSample << " In_Residual=" << inResidual << " Out_Residual=" << outResidual << endl;
                    }
                    else
                    {
                    	// Right now, I'm just interested in mismatches
                        //cout << "***Match at Index:[" << (Rows * ((z) * Columns + (y)) + (x)) << "] x=" << x << ", y="
                        //     << y << ", z=" << z;
                        //cout << "...In_Sample=" << inSample << " Out_Sample=" << outSample << " In_Residual=" << inResidual << " Out_Residual=" << outResidual << endl;
                    }

                }
            }
        }
    }

    delete []sensorResiduals;
    delete []groundResiduals;

    //===============================================================================
	#endif


    return 0;
}
