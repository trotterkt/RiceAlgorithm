/*
 * rice_main.cpp
 *
 *  Created on: Aug 16, 2015
 *      Author: trotterkt
 */



#include "Sensor.h"
#include "Predictor.h"

using namespace RiceAlgorithm;

int main(int argc, char *argv[])
{
    const int Rows(1024);
    const int Columns(1024);
    const int Bands(6);

	Sensor landsat("Landsat_agriculture-u16be-6x1024x1024", Rows, Columns, Bands);
	landsat.getSamples(1);
	landsat.process();

	Predictor predictSatellite(Rows, Columns, Bands);

	//:TODO: This should be Virtual TLM - packets in the file should only be available
	// at a certain rate: maybe a timed loop and a message when available.
	//predictSatellite.readSamples("Landsat_agriculture-u16be-6x1024x1024.raw");


}
