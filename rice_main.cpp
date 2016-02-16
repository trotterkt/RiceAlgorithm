/*
 * rice_main.cpp
 *
 *  Created on: Aug 16, 2015
 *      Author: trotterkt
 */


#include <iostream>
#include "Sensor.h"
#include "Predictor.h"
#include "Timing.h"


    
using namespace RiceAlgorithm;
using namespace std;

int main(int argc, char *argv[])
{
    const int Rows(1024);
    const int Columns(1024);
    const int Bands(6);

    cout.precision(4);

    cout << "Compressing Landsat_agriculture-u16be-6x1024x1024..." << endl;
   
	Sensor landsat("Landsat_agriculture-u16be-6x1024x1024", Rows, Columns, Bands);
	landsat.getSamples(1);
	
    timestamp_t t0 = getTimestamp();

	landsat.process();

    timestamp_t t1 = getTimestamp();
    
    double seconds = (t1 - t0) / MicroSecondsPerSecond;
    
    cout << "Compression processing time ==> " << fixed << seconds << " seconds"<< endl;

	Predictor predictSatellite(Rows, Columns, Bands);

	//:TODO: This should be Virtual TLM - packets in the file should only be available
	// at a certain rate: maybe a timed loop and a message when available.
	//predictSatellite.readSamples("Landsat_agriculture-u16be-6x1024x1024.raw");

    return 0;
}
