/*
 * rice_main.cpp
 *
 *  Created on: Aug 16, 2015
 *      Author: trotterkt
 */



#include "Predictor.h"

using namespace RiceAlgorithm;

int main(int argc, char *argv[])
{
    const int Rows(1024);
    const int Columns(1024);
    const int Bands(6);

	Predictor predictSatellite(Rows, Columns, Bands);

	predictSatellite.readSamples("Landsat_agriculture-u16be-6x1024x1024.raw");
}
