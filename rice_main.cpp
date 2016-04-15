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


    
using namespace RiceAlgorithm;
using namespace std;

int main(int argc, char *argv[])
{
    // These parameters are what is utilized for LandSat
    const int Rows(1024);
    const int Columns(1024);
    const int Bands(6);

    cout.precision(4);

    cout << "Compressing Landsat_agriculture-u16be-6x1024x1024..." << endl;
   
    FileBasedImagePersistence image("Landsat_agriculture-u16be-6x1024x1024", Rows, Columns, Bands);

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


    // Check what I have
    //===============================================================================
    #define matrixBsqIndexCheck(matrix, x, y, z) matrix[Rows*((z)*Columns + (y)) + (x)]
    int count = 0;
    for(int x=0; x<Rows; x++)
    {
        for(int y=0; y<Columns; y++)
        {
            for(int z=0; z<Bands; z++)
            {
                ushort inSample = matrixBsqIndexCheck(landsat.getSamples(), x, y, z);
                ushort outSample = matrixBsqIndexCheck(image.getDecodedData(), x, y, z);

                count++;

                if(inSample != outSample)
                {
                    cout << "Mismatch at Index:[" << (Rows*((z)*Columns + (y)) + (x)) << "] x=" << x << ", y=" << y << ", z=" << z;
                }
                else
                {
                    cout << "***Match at Index:[" << (Rows*((z)*Columns + (y)) + (x)) << "] x=" << x << ", y=" << y << ", z=" << z;
                }

                cout << "...In_Sample=" << inSample << " Out_Sample=" << outSample << endl;

                //if(count > 100) exit(0);

            }
        }
    }
    //===============================================================================

    return 0;
}
