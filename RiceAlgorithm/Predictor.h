/*
 * Predictor.h
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#ifndef PREDICTOR_H_
#define PREDICTOR_H_

namespace RiceAlgorithm
{

const unsigned int PredictionBands(5);
const unsigned int PredictionWeightResolution(15);
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
	public:
		Predictor(unsigned int x, unsigned int y, unsigned int z);
		virtual ~Predictor();

		bool readSamples(char* fileName);

	private:
		unsigned short int * mySamples;
		unsigned short int * myResiduals;
		unsigned int myX;
		unsigned int myY;
		unsigned int myZ;

		/// given the local differences and the samples, it computes the scaled predicted
		/// sample value
		int compute_predicted_sample(unsigned int x, unsigned int y, unsigned int z,
				unsigned int s_min, unsigned int s_mid, unsigned int s_max,
				unsigned short int * samples, int * weights);


		int local_sum(unsigned int x, unsigned int y, unsigned int z, unsigned short int * samples);
		long long mod_star(long long arg, long long op);

		int compute_local_differences(int *** local_differences, unsigned short int * samples);

		unsigned short int compute_mapped_residual(unsigned int x, unsigned int y, unsigned int z,
				unsigned int s_min, unsigned int s_mid, unsigned int s_max,
				unsigned short int * samples, int scaled_predicted);
		void init_weights(int * weights, unsigned int z);
		void update_weights(int *weights, unsigned int x, unsigned int y,
				unsigned int z, int error, int ** local_differences,
				unsigned short int * samples);
		int get_central_difference(int * central_difference, unsigned short int * samples, unsigned int x, unsigned int y, unsigned int z);

		int get_directional_difference(int directional_difference[3], unsigned short int * samples, unsigned int x, unsigned int y, unsigned int z);

#define MATRIX_BSQ_INDEX(matrix, x, y, z) matrix[myX*((z)*myY + (y)) + (x)]

};

} /* namespace RiceAlgorithm */

#endif /* PREDICTOR_H_ */
