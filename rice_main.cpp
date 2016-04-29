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
    // These parameters are what is utilized for LandSat
    const int Rows(1024);
    const int Columns(1024);
    const int Bands(6);

    cout.precision(4);

    cout << "Compressing Landsat_agriculture-u16be-6x1024x1024..." << endl;
   
    FileBasedImagePersistence image("Landsat_agriculture-u16be-6x1024x1024", Rows, Columns, Bands);

	//***********************************************
	watchEncoded1 = image.getEncodedData()[700];
	watchEncoded2 = image.getEncodedData()[750];
	//***********************************************

    // Construct my LandSat sensor, which performs the compression of the supplied
    // raw image data per the Rice algorithm
	Sensor landsat(&image, Rows, Columns, Bands);

    timestamp_t t0 = getTimestamp();

    // Initiate the Rice algorithm compression
	landsat.process();

    timestamp_t t1 = getTimestamp();

    cout << "Compression processing time ==> " << fixed << getSecondsDiff(t0, t1) << " seconds"<< endl;

    // Write out the encoded data. This is outside of the compression processing
    image.writeEncodedData();


    // Kick off the associated decompression
    landsat.getGround()->process();

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

                if(index >= 0 && index <= 1090)
                {
                    ushort inSample = matrixBsqIndexCheck(landsat.getSamples(), x, y, z);
                    ushort outSample = matrixBsqIndexCheck(image.getDecodedData(), x, y, z);

                    ushort inResidual = matrixBsqIndexCheck(sensorResiduals, x, y, z);
                    ushort outResidual = matrixBsqIndexCheck(groundResiduals, x, y, z);


                    count++;

                    //if (inSample != outSample)
                    if (inResidual != outResidual)
                    {
                        cout << "Mismatch at Index:[" << index << "] x=" << x << ", y=" << y << ", z=" << z << " --Delta=" << (outSample - inSample);
                    }
                    else
                    {
                        cout << "***Match at Index:[" << (Rows * ((z) * Columns + (y)) + (x)) << "] x=" << x << ", y="
                             << y << ", z=" << z;
                    }

                    cout << "...In_Sample=" << inSample << " Out_Sample=" << outSample << " In_Residual=" << inResidual << " Out_Residual=" << outResidual << endl;
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
