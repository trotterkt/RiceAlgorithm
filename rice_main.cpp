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

    return 0;
}
