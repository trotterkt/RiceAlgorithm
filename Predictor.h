/*
 * Predictor.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#include <stddef.h>
#include <sys/types.h>
#include <vector>
#include <AdaptiveEntropyEncoder.h>


#ifndef PREDICTOR_H_
#define PREDICTOR_H_

namespace RiceAlgorithm
{

const unsigned int PredictionBands(5);
const unsigned int UserInputPredictionBands(15);
const unsigned int PredictionWeightResolution(14);
const unsigned int PredictionWeightInitResolution(0);
const int PredictionWeightInitial(-6);
const int PredictionWeightFinal(-6);
const int PredictionWeightInterval(32);
const unsigned int PredictionNeighborSum(0);
const unsigned int PredictionFull(1);
const unsigned int RegisterSize(32);
const unsigned char DynamicRange(16);
const unsigned int ReferenceInterval(1000);

// For details on the predictor, refer to Sec 4 of CCSDS 123.0-R-1,
// Lossless Multispectral & Hyperspectral Image Compression

// Note that there need not be one particular type of Predictor used,
// so depending on the application, we might want to swap this out

class Predictor
{
	private:

    public:
        Predictor(unsigned int x, unsigned int y, unsigned int z);
        virtual ~Predictor();

        ushort* getSamples() { return mySamples; }

        ushort* getResiduals(ushort* samples);

        size_t getSizeOfSamples() { return ( sizeof(ushort) * (myXDimension * myYDimension * myZDimension)); }



    private:
        ushort* mySamples;
        int* myWeights;
        ushort * myResiduals;
        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        // based on the local differences and the samples, determine the scaled predicted
        // sample value
        int calculatePredictedSample(unsigned int x, unsigned int y, unsigned int z, unsigned int sampleMinimum,
                                     unsigned int sampleMidway, unsigned int sampleMaximum);

        // Find the local sum for the sample index
        int getLocalSum(unsigned int x, unsigned int y, unsigned int z);

        // Computes the mod*R of a number according to the definition of the
        // red book ==> mod*R[x] = ((x + 2^R-1) mod 2^R)-2^(R-1)
        long long modRstar(long long arg, long long op);

        // Given the scaled predicted sample value, map it to an unsigned value
        // enabling it to be represented with D bits
        unsigned short int computeMappedResidual(unsigned int x, unsigned int y, unsigned int z, unsigned int sampleMinimum,
                                                   unsigned int sampleMidway, unsigned int sampleMaximum, int scaledPredicted);
        void initializeWeights(unsigned int z);
        void updateWeights(unsigned int x, unsigned int y, unsigned int z, int error);

        int getCentralDifference(int * centralDifference, unsigned int x, unsigned int y, unsigned int z);

        int getDirectionalDifference(int directionalDifferenceVector[3], unsigned int x, unsigned int y, unsigned int z);

		#define matrixBsqIndex(matrix, x, y, z) matrix[myXDimension*((z)*myYDimension + (y)) + (x)]

};

} /* namespace RiceAlgorithm */

#endif /* PREDICTOR_H_ */
