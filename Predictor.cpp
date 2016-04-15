/*
 * Predictor.cpp
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#include <Predictor.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <stdio.h>
#include <iostream>

using namespace std;

namespace RiceAlgorithm
{

Predictor::Predictor(unsigned int x, unsigned int y, unsigned int z) :
        myXDimension(x), myYDimension(y), myZDimension(z), mySamples(0)
{
    myResiduals = reinterpret_cast<ushort*>(new ushort[myXDimension * myYDimension * myZDimension]);

    int weightLength = PredictionBands + (PredictionFull != 0 ? 3 : 0);
    myWeights = reinterpret_cast<int *>(new int[weightLength]);
}

Predictor::~Predictor()
{
    if(myResiduals)
    {
        delete[] myResiduals;
        myResiduals = 0;
    }

    if(myWeights)
    {
        delete[] myWeights;
        myWeights = 0;
    }
}

ushort* Predictor::getResiduals(ushort* samples)
{

    if(mySamples)
    {
        delete [] mySamples;
        mySamples = 0;
    }

    mySamples = samples;

    //============================================================================================================

    // This defines the range of the signal value as described in the Blue Book
    // for 16-bit samples
    unsigned int sampleMinimum = 0;
    unsigned int sampleMaximum = (0x1 << DynamicRange) - 1;
    unsigned int sampleMidway = 0x1 << (DynamicRange - 1);


    // Now actually it goes over the various samples and it computes the prediction
    // residual for each of them
    // Note that, for each band, the element in position (0, 0) is not predicted
    for (unsigned int z = 0; z < myZDimension; z++)
    {
        for (unsigned int y = 0; y < myYDimension; y++)
        {
            for (unsigned int x = 0; x < myXDimension; x++)
            {
                int predictedSample = 0;
                unsigned short int mappedResidual = 0;
                int error = 0;

                // prediction of the current sample and saving the residual
                predictedSample = calculatePredictedSample(x, y, z, sampleMinimum, sampleMidway, sampleMaximum);


                mappedResidual = computeMappedResidual(x, y, z, sampleMinimum, sampleMidway, sampleMaximum, predictedSample);

                matrixBsqIndex(myResiduals, x, y, z) = mappedResidual;
                if (x == 0 && y == 0)
                {
                    //  weights initialization
                    initializeWeights(z);
                }
                else
                {
                    // Update the weights and prepare next sample prediction
                    error = 2 * (matrixBsqIndex(mySamples, x, y, z)) - predictedSample;
                    updateWeights(x, y, z, error);
                }

            }
        }
    }

	return myResiduals;
}


void Predictor::getSamples(ushort* residualsPtr, ushort* samples)
{
    if(mySamples)
    {
        delete [] mySamples;
    }

    mySamples = samples;


    if(myResiduals)
    {
        delete [] myResiduals;
    }

    myResiduals = residualsPtr;


    //============================================================================================================

    // This defines the range of the signal value as described in the Blue Book
    // for 16-bit samples
    unsigned int sampleMinimum = 0;
    unsigned int sampleMaximum = (0x1 << DynamicRange) - 1;
    unsigned int sampleMidway = 0x1 << (DynamicRange - 1);


    // Now actually it goes over the various residuals and it computes the applicable
    // raw sample for each of them
    for (unsigned int z = 0; z < myZDimension; z++)
    {
        for (unsigned int y = 0; y < myYDimension; y++)
        {
            for (unsigned int x = 0; x < myXDimension; x++)
            {
                int predictedSample = 0;
                ushort unmappedSample = 0;
                int error = 0;

                // prediction of the current sample and saving the residual
                predictedSample = calculatePredictedSample(x, y, z, sampleMinimum, sampleMidway, sampleMaximum);


                unmappedSample = computeUnmappedSample(x, y, z, sampleMinimum, sampleMidway, sampleMaximum, predictedSample);

                matrixBsqIndex(mySamples, x, y, z) = unmappedSample;
                // For Debugging
                //matrixBsqIndex(mySamples, x, y, z) = myXDimension*((z)*myYDimension + (y)) + (x);
                if (x == 0 && y == 0)
                {
                    //  weights initialization
                    initializeWeights(z);
                }
                else
                {
                    // Update the weights and prepare next sample prediction
                    error = 2 * (matrixBsqIndex(mySamples, x, y, z)) - predictedSample;
                    updateWeights(x, y, z, error);
                }
            }
        }
    }
}

int Predictor::calculatePredictedSample(unsigned int x, unsigned int y, unsigned int z, unsigned int sampleMinimum,
                                        unsigned int sampleMidway, unsigned int sampleMaximum)
{
    long long scaledPredicted = 0;
    long long diffPredicted = 0;
    int i = 0;

    if (x > 0 || y > 0)
    {
        long long localSum = 0;

        // predicted local difference
        if (z > 0)
        {
            int currentPredictionBands = z < PredictionBands ? z : PredictionBands;
            for (i = 0; i < currentPredictionBands; i++)
            {
                int centralDifference = 0;
                if (getCentralDifference(&centralDifference, x, y, z - i - 1) < 0)
                {
                    cerr << "Error in getting the central differences for band " << (z - i) << endl;
                }


                diffPredicted += ((long long) myWeights[i]) * (long long) centralDifference;

            }
        }
        if (PredictionFull != 0)
        {
            int directionalDifferenceVector[3];
            if (getDirectionalDifference(directionalDifferenceVector, x, y, z) < 0)
            {
                cerr << "Error in getting the directional differences" << endl;
            }

            for (i = 0; i < 3; i++)
            {
                diffPredicted += ((long long) myWeights[PredictionBands + i]) * (long long) directionalDifferenceVector[i];
            }
        }

        // scaled predicted sample
        localSum = getLocalSum(x, y, z);
        scaledPredicted = modRstar(diffPredicted + ((localSum - 4 * sampleMidway) << PredictionWeightResolution), RegisterSize);
        scaledPredicted = scaledPredicted >> (PredictionWeightResolution + 1);
        scaledPredicted = scaledPredicted + 1 + 2 * sampleMidway;
        if (scaledPredicted < 2 * sampleMinimum) scaledPredicted = 2 * sampleMinimum;
        if (scaledPredicted > (2 * sampleMaximum + 1)) scaledPredicted = (2 * sampleMaximum + 1);
    }
    else
    {
        if (z == 0 || PredictionBands == 0)
        {
            scaledPredicted = 2 * sampleMidway;
        }
        else
        {
            scaledPredicted = 2 * (matrixBsqIndex(mySamples, 0, 0, z - 1));
        }
    }

    return (int) scaledPredicted;
}

int Predictor::getLocalSum(unsigned int x, unsigned int y, unsigned int z)
{
    unsigned int sum = 0;

    if (x == 0 && y == 0)
    {
        cerr << "Error, called local_sum for band " << z << " with x=0, y=0\n\n" << endl;
        return 0x80000000;
    }

    // Using column-oriented local sums
    if (y > 0)
    {
    	sum = 4 * matrixBsqIndex(mySamples, x, y - 1, z);
    }
    else
    {
        sum = 4 * matrixBsqIndex(mySamples, x - 1, y, z);
    }

    return sum;
}

long long Predictor::modRstar(long long arg, long long op)
{
    // This is largely based on example provided on CCSDS site

    long long power2 = ((long long) 0x1) << (op - 1);

    // trick of shifting not of the op quantity altogether as
    // when op == 64 no shift is actually performed by the system
    return ((arg + power2) - (((((arg + power2) >> (op - 1) >> 1)) << (op - 1)) << 1)) - power2;
}


ushort Predictor::computeMappedResidual(unsigned int x, unsigned int y, unsigned int z,
                                        unsigned int sampleMinimum, unsigned int sampleMidway, unsigned int sampleMaximum,
                                        int scaledPredicted)
{
    unsigned short int mapped = 0;
    int delta = ((int) matrixBsqIndex(mySamples, x, y, z)) - scaledPredicted / 2;
    unsigned int omega = scaledPredicted / 2 - sampleMinimum;
    unsigned int absDelta = delta < 0 ? (-1 * delta) : delta;
    int sign_scaled = (scaledPredicted & 0x1) != 0 ? -1 : 1;

    if (omega > sampleMaximum - scaledPredicted / 2)
    {
        omega = sampleMaximum - scaledPredicted / 2;
    }

    if (absDelta > omega)
    {
        mapped = absDelta + omega;
    }
    else if ((sign_scaled * delta) <= omega && (sign_scaled * delta) >= 0)
    {
        mapped = 2 * absDelta;
    }
    else
    {
        mapped = 2 * absDelta - 1;
    }

    return mapped;
}

ushort Predictor::computeUnmappedSample(unsigned int x, unsigned int y, unsigned int z,
                                        unsigned int sampleMinimum, unsigned int sampleMidway, unsigned int sampleMaximum,
                                        int scaledPredicted)
{
    ushort sample = 0;
    unsigned int omega = (scaledPredicted / 2) - sampleMinimum;
    unsigned int selectedOmega = 0;
    int delta = 0;


    if (omega > sampleMaximum - (scaledPredicted / 2))
    {
        omega = sampleMaximum - (scaledPredicted / 2);
        selectedOmega = 1;
    }

    if(matrixBsqIndex(myResiduals, x, y, z) > 2*omega)
    {
        if(selectedOmega == 0)
        {
            delta = matrixBsqIndex(myResiduals, x, y, z) - omega;
        }
        else
        {
            delta = omega - matrixBsqIndex(myResiduals, x, y, z);
        }
    }
    else
    {
        if((matrixBsqIndex(myResiduals, x, y, z) & 0x1) == 0)
        {
            int sign_scaled = (scaledPredicted & 0x1) != 0 ? -1 : 1;
            delta = sign_scaled*(matrixBsqIndex(myResiduals, x, y, z)/2);
        }
        else
        {
            int sign_scaled = (scaledPredicted & 0x1) != 0 ? 1 : -1;
            delta = sign_scaled*((matrixBsqIndex(myResiduals, x, y, z) + 1)/2);
        }
    }

    sample = delta + (scaledPredicted / 2);


    return sample;
}

void Predictor::initializeWeights(unsigned int z)
{
	unsigned int i = 0;

	// default weights initialization
	if (PredictionBands > 0)
	{
		myWeights[0] = 7 << (PredictionWeightResolution - 3);
		for (i = 1; i < PredictionBands; i++)
		{
			myWeights[i] = myWeights[i - 1] >> 3;
		}
	}
	if (PredictionFull != 0)
	{
		for (i = 0; i < 3; i++)
		{
			myWeights[PredictionBands + i] = 0;
		}
	}
}

void Predictor::updateWeights(unsigned int x, unsigned int y, unsigned int z, int error)
{
    int i = 0;
    int weightLimit = 0x1 << (PredictionWeightResolution + 2);
    int signError = error < 0 ? -1 : 1;
    int scaling_exp = PredictionWeightInitial + ((int) (y * myXDimension + x - myXDimension)) / PredictionWeightInterval;
    if (scaling_exp < PredictionWeightInitial) scaling_exp = PredictionWeightInitial;
    if (scaling_exp > PredictionWeightFinal) scaling_exp = PredictionWeightFinal;
    scaling_exp += DynamicRange - PredictionWeightResolution;

    // Update the weights; they are saved in the weights matrix such as
    // the first index of the matrix is the spectral band and the other elements are
    // the various weights: pred_1, pred_2, pred_p, N, W, NW.
    if (z > 0)
    {
        int currentPredictionBands = z < PredictionBands ? z : PredictionBands;

        for (i = 0; i < currentPredictionBands; i++)
        {
            int centralDifference = 0;
            if (getCentralDifference(&centralDifference, x, y, z - i - 1) < 0)
            {
                cerr << "Error in getting the central differences for band " << (z - i);
            }

            if (scaling_exp > 0)
            {
            	myWeights[i] = myWeights[i] + ((((signError * centralDifference) >> scaling_exp) + 1) >> 1);
            }
            else
            {
            	myWeights[i] = myWeights[i] + ((((signError * centralDifference) << -1 * scaling_exp) + 1) >> 1);
            }
            if (myWeights[i] < (-1 * weightLimit))
            {
            	myWeights[i] = -1 * weightLimit;
            }
            if (myWeights[i] > (weightLimit - 1))
            {
            	myWeights[i] = weightLimit - 1;
            }
        }
    }

    if (PredictionFull != 0)
    {
        int directionalDifferenceVector[3];
        if (getDirectionalDifference(directionalDifferenceVector, x, y, z) < 0)
        {
            cerr << "Error in getting the directional differences";
        }

        for (i = 0; i < 3; i++)
        {
            if (scaling_exp > 0)
            {
            	myWeights[PredictionBands + i] = myWeights[PredictionBands + i]
                        + ((((signError * directionalDifferenceVector[i]) >> scaling_exp) + 1) >> 1);
            }
            else
            {
            	myWeights[PredictionBands + i] = myWeights[PredictionBands + i]
                        + ((((signError * directionalDifferenceVector[i]) << -1 * scaling_exp) + 1) >> 1);
            }

            if (myWeights[PredictionBands + i] < (-1 * weightLimit))
            {
            	myWeights[PredictionBands + i] = -1 * weightLimit;
            }

            if (myWeights[PredictionBands + i] > (weightLimit - 1))
            {
            	myWeights[PredictionBands + i] = weightLimit - 1;
            }
        }
    }
}

int Predictor::getCentralDifference(int * centralDifference, unsigned int x, unsigned int y, unsigned int z)
{

    // Central difference computed: it is common both to reduced and
    // full prediction mode
    if (y == 0 && x == 0)
    {
        *centralDifference = 0;
    }
    else
    {
        int localSum = getLocalSum(x, y, z);

        if (localSum == 0x80000000)
        {
            return -1;
        }

        *centralDifference = 4 * matrixBsqIndex(mySamples, x, y, z) - localSum;
    }

    return 0;
}

int Predictor::getDirectionalDifference(int directionalDifferenceVector[3], unsigned int x, unsigned int y, unsigned int z)
{
    int localSum = getLocalSum(x, y, z);

    if (localSum == 0x80000000)
    {
        return -1;
    }

    // The differences in the directionalDifferenceVector are
    // in this order: central, north, west, north-west
    if (y > 0)
    {
        directionalDifferenceVector[0] = 4 * matrixBsqIndex(mySamples, x, y - 1, z) - localSum;
        if (x > 0)
        {
            directionalDifferenceVector[1] = 4 * matrixBsqIndex(mySamples, x - 1, y, z) - localSum;
            directionalDifferenceVector[2] = 4 * matrixBsqIndex(mySamples, x - 1, y - 1, z) - localSum;
        }
        else
        {
            directionalDifferenceVector[1] = directionalDifferenceVector[0];
            directionalDifferenceVector[2] = directionalDifferenceVector[0];
        }
    }
    else
    {
        directionalDifferenceVector[0] = 0;
        directionalDifferenceVector[1] = 0;
        directionalDifferenceVector[2] = 0;
    }

    return 0;
}

} /* namespace RiceAlgorithm */
