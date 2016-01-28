/*
 * Predictor.h
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
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
const unsigned int PredictionWeightResolution(14);
const unsigned int PredictionWeightInitResolution(0);
const int PredictionWeightInitial(-6);
const int PredictionWeightFinal(-6);
const int PredictionWeightInterval(32);
const unsigned int PredictionNeighborSum(0);
const unsigned int PredictionFull(1);
const unsigned int RegisterSize(32);
const unsigned char DynamicRange(16);

class Predictor
{
	private:

    public:
        Predictor(unsigned int x, unsigned int y, unsigned int z);
        virtual ~Predictor();

        bool readSamples(ushort* samples);

        ushort* getSamples() { return mySamples; }

        ushort* getResiduals();

        size_t getSizeOfSamples() { return ( sizeof(ushort) * (myXDimension * myYDimension * myZDimension)); }



    private:
        ushort* mySamples;
        int* myWeights;
        ushort * myResiduals;
        unsigned int myXDimension;
        unsigned int myYDimension;
        unsigned int myZDimension;

        /// given the local differences and the samples, it computes the scaled predicted
        /// sample value
        int compute_predicted_sample(unsigned int x, unsigned int y, unsigned int z, unsigned int s_min,
                                     unsigned int s_mid, unsigned int s_max);

        int local_sum(unsigned int x, unsigned int y, unsigned int z);
        long long mod_star(long long arg, long long op);

        unsigned short int computeMappedResidual(unsigned int x, unsigned int y, unsigned int z, unsigned int s_min,
                                                   unsigned int s_mid, unsigned int s_max, int scaled_predicted);
        void initializeWeights(unsigned int z);
        void updateWeights(unsigned int x, unsigned int y, unsigned int z, int error);

        int get_central_difference(int * central_difference, unsigned int x, unsigned int y, unsigned int z);

        int get_directional_difference(int directional_difference[3], unsigned int x, unsigned int y, unsigned int z);

		#define MATRIX_BSQ_INDEX(matrix, x, y, z) matrix[myXDimension*((z)*myYDimension + (y)) + (x)]

};

} /* namespace RiceAlgorithm */

#endif /* PREDICTOR_H_ */
