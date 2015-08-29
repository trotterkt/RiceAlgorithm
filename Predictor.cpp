/*
 * Predictor.cpp
 *
 *  Created on: Aug 14, 2015
 *      Author: trotterkt
 */

#include "Predictor.h"
#include <fstream>

using namespace std;

namespace RiceAlgorithm
{

Predictor::Predictor(unsigned int x, unsigned int y, unsigned int z) :
		myX(x), myY(y), myZ(z)
{
	mySamples = reinterpret_cast<unsigned short int*>(new unsigned short int[myX * myY * myZ]);
	myResiduals = reinterpret_cast<unsigned short int*>(new unsigned short int[myX * myY * myZ]);

}

Predictor::~Predictor()
{
	delete[] mySamples;
	delete[] myResiduals;
}

bool Predictor::readSamples(char* fileName)
{
	//:TODO: Need to read in samples from file
    fstream sampleStream;
	sampleStream.open(fileName, fstream::out | fstream::binary);

	unsigned int s_min = 0;
	unsigned int s_max = (0x1 << DynamicRange) - 1;
	unsigned int s_mid = 0x1 << (DynamicRange - 1);

	int weights_len = PredictionBands + (PredictionFull != 0 ? 3 : 0);

	int* weights = reinterpret_cast<int *>(new int[weights_len]);
	int ** local_differences = NULL;

	// Now actually it goes over the various samples and it computes the prediction
	// residual for each of them
	// Note that, for each band, the element in position (0, 0) is not predicted
	for (unsigned int z = 0; z < myZ; z++)
	{
		for (unsigned int y = 0; y < myY; y++)
		{
			for (unsigned int x = 0; x < myX; x++)
			{
				int predicted_sample = 0;
				unsigned short int mapped_residual = 0;
				int error = 0;

				// prediction of the current sample and saving the residual
				predicted_sample = compute_predicted_sample(x, y, z, s_min, s_mid, s_max,
						 mySamples, weights);

				mapped_residual = compute_mapped_residual(x, y, z, s_min, s_mid,
						s_max, mySamples, predicted_sample);
				MATRIX_BSQ_INDEX(myResiduals, x, y, z) = mapped_residual;
				if (x == 0 && y == 0)
				{
					//  weights initialization
					init_weights(weights, z);
				}
				else
				{
					// finally I can update the weights, preparing for the prediction of the next sample
					error = 2
					* (MATRIX_BSQ_INDEX(mySamples, x, y, z))
					- predicted_sample;
					update_weights(weights, x, y, z, error,
							local_differences, mySamples);
				}
			}
		}
	}

	sampleStream.close();

	return 0;
}



/// given the local differences and the samples, it computes the scaled predicted
/// sample value
	int Predictor::compute_predicted_sample(unsigned int x, unsigned int y, unsigned int z, unsigned int s_min, unsigned int s_mid, unsigned int s_max,
			unsigned short int * samples, int * weights)
	{
	long long scaled_predicted = 0;
	long long diff_predicted = 0;
	int i = 0;

	//--input Landsat_agriculture-u16be-6x1024x1024.raw
	//--output Landsat_agriculture-u16be-6x1024x1024.comp --rows 1024 --columns 1024 --bands 6
	//--in_format BSQ  --dyn_range 16  --word_len 1 --out_format BSQ  --sample_adaptive
	//--u_max 18 --y_star 6 --y_0 1  --k 7 --pred_bands 15 --full --reg_size 32
	//--w_resolution 14 --w_interval 32 --w_initial -6 --w_final -6

	if (x > 0 || y > 0)
	{
		long long local_sum_temp = 0;

		// predicted local difference
		if (z > 0)
		{
			int cur_pred_bands =
					z < PredictionBands ?
							z : PredictionBands;
			for (i = 0; i < cur_pred_bands; i++)
			{
				int central_difference = 0;
				if(get_central_difference(&central_difference, mySamples, x, y, z - i - 1) < 0)
				{
					fprintf(stderr, "Error in getting the central differences for band %d", z -i);
				}

				diff_predicted += ((long long)weights[i])*(long long)central_difference;
			}
		}
		if (PredictionFull != 0)
		{
			int directional_difference[3];
			if(get_directional_difference(directional_difference, mySamples, x, y, z) < 0)
			{
				fprintf(stderr, "Error in getting the directional differences");
			}

			for (i = 0; i < 3; i++)
			{
				diff_predicted += ((long long)weights[PredictionBands + i])*(long long)directional_difference[i];
			}
		}

		// scaled predicted sample
		local_sum_temp = local_sum(x, y, z, mySamples);
		scaled_predicted = mod_star(diff_predicted + ((local_sum_temp - 4 * s_mid) << PredictionWeightResolution),
									RegisterSize);
		scaled_predicted = scaled_predicted
				>> (PredictionWeightResolution + 1);
		scaled_predicted = scaled_predicted + 1 + 2 * s_mid;
		if (scaled_predicted < 2 * s_min)
			scaled_predicted = 2 * s_min;
		if (scaled_predicted > (2 * s_max + 1))
			scaled_predicted = (2 * s_max + 1);
	}
	else
	{
		if (z == 0 || PredictionBands == 0)
		{
			scaled_predicted = 2 * s_mid;
		}
		else
		{
			scaled_predicted = 2
					* (MATRIX_BSQ_INDEX(mySamples, 0, 0, z - 1));
		}
	}

	return (int) scaled_predicted;
}


/// Computes the local sum for the given sample index
int Predictor::local_sum(unsigned int x, unsigned int y, unsigned int z,
		unsigned short int * samples)
{
	unsigned int sum = 0;

#ifndef NDEBUG
	if (x == 0 && y == 0)
	{
		fprintf(stderr, "Error, called local_sum for band %d with x=0, y=0\n\n", z);
		return 0x80000000;
	}
#endif

	if (y > 0)
		sum = 4 * MATRIX_BSQ_INDEX(samples, x, y - 1, z);
	else
		sum = 4 * MATRIX_BSQ_INDEX(samples, x - 1, y, z);

	return sum;
}

/// Computes the mod*R of a number according to the definition of the
/// blue book
long long Predictor::mod_star(long long arg, long long op)
{
	long long power2 = ((long long) 0x1) << (op - 1);
	// I have to use the trick of shifting not of the op quantity altogether as
	// when op == 64 no shift is actually performed by the system
	return ((arg + power2) - (((((arg + power2) >> (op - 1) >> 1)) << (op - 1)) << 1)) - power2;
}

/// Computes the local differences for the whole image and saves them in the local_differences
/// array; note that the image input to this procedure is assumed to be in BSQ format
int Predictor::compute_local_differences(int *** local_differences, unsigned short int * samples)
{
	unsigned int x = 0, y = 0, z = 0;

	// First I have to allocate the memory to hold the result
	if (PredictionFull != 0)
		*local_differences = reinterpret_cast<int **>(new int[4]);
	else
		*local_differences = reinterpret_cast<int **>(new int);

	if (*local_differences == NULL)
	{
		fprintf(stderr, "Error in allocating memory for building the local differences matrices\n");
		return -1;
	}

	if ((*local_differences)[0] = reinterpret_cast<int *>(new int[myX * myY * myZ]))
	{
		fprintf(stderr, "Error in allocating %d bytes for holding the local differences matrix\n",
				sizeof(int) * myX * myY * myZ);
		return -1;
	}
	if (PredictionFull != 0)
	{
		int i = 0;
		for (i = 1; i < 4; i++)
		{
			if (((*local_differences)[i] = reinterpret_cast<int *>(new int[myX * myY * myZ]))
					== NULL)
			{
				fprintf(stderr,
						"Error in allocating %d bytes for holding the local differences matrix %d\n",
						sizeof(int) * myX * myY * myZ);
				return -1;
			}
		}
	}

	// Ok, now I can actually start the computation
	// Central difference computed: it is common both to reduced and
	// full prediction mode
	for (z = 0; z < myZ; z++)
	{
		for (y = 0, x = 1; y < myY; y++, x = 0)
		{
			for (; x < myX; x++)
			{
				int local_sum_temp = local_sum(x, y, z, mySamples);
#ifndef NDEBUG
				if (local_sum_temp == 0x80000000)
				{
					return -1;
				}
#endif
				MATRIX_BSQ_INDEX((*local_differences)[0], x, y, z)= 4*MATRIX_BSQ_INDEX(mySamples, x, y, z) - local_sum_temp;
			}
		}
	}
	if (PredictionFull != 0)
	{
		//full prediction mode, the differences in the local_differences vector are
		//in this order: central, north, west, north-west
		for (z = 0; z < myZ; z++)
		{
			for (y = 0; y < myY; y++)
			{
				for (x = 0; x < myX; x++)
				{
					if (y > 0)
					{
						int local_sum_temp = local_sum(x, y, z, mySamples);
#ifndef NDEBUG
						if (local_sum_temp == 0x80000000)
						{
							return -1;
						}
#endif
						MATRIX_BSQ_INDEX((*local_differences)[1], x, y, z)= 4*MATRIX_BSQ_INDEX(samples, x, y - 1, z) - local_sum_temp;
						if (x > 0)
						{
							MATRIX_BSQ_INDEX((*local_differences)[2], x, y, z)= 4*MATRIX_BSQ_INDEX(samples, x - 1, y, z) - local_sum_temp;
							MATRIX_BSQ_INDEX((*local_differences)[3], x, y, z) = 4*MATRIX_BSQ_INDEX(samples, x - 1, y - 1, z) - local_sum_temp;
						}
						else
						{
							MATRIX_BSQ_INDEX((*local_differences)[2], 0, y, z) = MATRIX_BSQ_INDEX((*local_differences)[1], 0, y, z);
							MATRIX_BSQ_INDEX((*local_differences)[3], 0, y, z) = MATRIX_BSQ_INDEX((*local_differences)[1], 0, y, z);
						}
					}
					else
					{
						MATRIX_BSQ_INDEX((*local_differences)[1], x, 0, z) = 0;
						MATRIX_BSQ_INDEX((*local_differences)[2], x, 0, z) = 0;
						MATRIX_BSQ_INDEX((*local_differences)[3], x, 0, z) = 0;
					}
				}
			}
		}
	}
	return 0;
}

/// Given the scaled predicted sample value it maps it to an unsigned value
/// enabling it to be represented with D bits
unsigned short int Predictor::compute_mapped_residual(unsigned int x, unsigned int y,
		unsigned int z, unsigned int s_min, unsigned int s_mid, unsigned int s_max,
		unsigned short int * samples, int scaled_predicted)
{
	unsigned short int mapped = 0;
	int delta = ((int) MATRIX_BSQ_INDEX(mySamples, x, y, z))
	- scaled_predicted / 2;
	unsigned int omega = scaled_predicted / 2 - s_min;
	unsigned int abs_delta = delta < 0 ? (-1 * delta) : delta;
	int sign_scaled = (scaled_predicted & 0x1) != 0 ? -1 : 1;

	if (omega > s_max - scaled_predicted / 2)
	{
		omega = s_max - scaled_predicted / 2;
	}

	if (abs_delta > omega)
	{
		mapped = abs_delta + omega;
	}
	else if ((sign_scaled * delta) <= omega && (sign_scaled * delta) >= 0)
	{
		mapped = 2 * abs_delta;
	}
	else
	{
		mapped = 2 * abs_delta - 1;
	}

	return mapped;
}

void Predictor::init_weights(int * weights, unsigned int z)
{
	int i = 0;

		// default weights initialization
		if (PredictionBands > 0)
		{
			weights[0] = 7 << (PredictionWeightResolution - 3);
			for (i = 1; i < PredictionBands; i++)
			{
				weights[i] = weights[i - 1] >> 3;
			}
		}
		if (PredictionFull != 0)
		{
			for (i = 0; i < 3; i++)
			{
				weights[PredictionBands + i] = 0;
			}
		}
	}

void Predictor::update_weights(int *weights, unsigned int x, unsigned int y,
		unsigned int z, int error, int ** local_differences,
		unsigned short int * samples)
{
	int i = 0;
	int weight_limit = 0x1 << (PredictionWeightResolution + 2);
	int sign_error = error < 0 ? -1 : 1;
	int scaling_exp = PredictionWeightInitial
			+ ((int) (y * myX + x - myX))
					/ PredictionWeightInterval;
	if (scaling_exp < PredictionWeightInitial)
		scaling_exp = PredictionWeightInitial;
	if (scaling_exp > PredictionWeightFinal)
		scaling_exp = PredictionWeightFinal;
	scaling_exp += DynamicRange - PredictionWeightResolution;

	// Now I can update the weights; they are saved in the weights matrix such as
	// the first index of the matrix is the spectral band and the other elements are
	// the various weights: pred_1, pred_2, pred_p, N, W, NW.
	if (z > 0)
	{
		int cur_pred_bands =
				z < PredictionBands ?
						z : PredictionBands;
		for (i = 0; i < cur_pred_bands; i++)
		{

			if (scaling_exp > 0)
			{

				weights[i] = weights[i]
						+ ((((sign_error
								* (MATRIX_BSQ_INDEX(local_differences[0],
										x, y, z - i - 1)))
								>> scaling_exp) + 1) >> 1);
			}
			else
			{

				weights[i] = weights[i]
						+ ((((sign_error
								* (MATRIX_BSQ_INDEX(local_differences[0],
										x, y, z - i - 1)))
								<< -1 * scaling_exp) + 1) >> 1);

			}
			if (weights[i] < (-1 * weight_limit))
			{
				weights[i] = -1 * weight_limit;
			}
			if (weights[i] > (weight_limit - 1))
			{
				weights[i] = weight_limit - 1;
			}
		}
	}
	if (PredictionFull != 0)
	{

		for (i = 0; i < 3; i++)
		{
			if (scaling_exp > 0)
			{
				weights[PredictionBands + i] =
						weights[PredictionBands + i]
								+ ((((sign_error
										* (MATRIX_BSQ_INDEX(
												local_differences[i + 1],
											 x, y, z)))
										>> scaling_exp) + 1) >> 1);

			}
			else
			{
				weights[PredictionBands + i] =
						weights[PredictionBands + i]
								+ ((((sign_error
										* (MATRIX_BSQ_INDEX(
												local_differences[i + 1],
											 x, y, z)))
										<< -1 * scaling_exp) + 1) >> 1);
			}
			if (weights[PredictionBands + i] < (-1 * weight_limit))
			{
				weights[PredictionBands + i] = -1 * weight_limit;
			}
			if (weights[PredictionBands + i] > (weight_limit - 1))
			{
				weights[PredictionBands + i] = weight_limit - 1;
			}
		}
	}
}


int Predictor::get_central_difference(int * central_difference, unsigned short int * samples, unsigned int x, unsigned int y, unsigned int z)
{

	// Central difference computed: it is common both to reduced and
	// full prediction mode
	if(y == 0 && x == 0)
	{
		*central_difference = 0;
	}
	else
	{
		int local_sum_temp = local_sum(x, y, z, samples);
#ifndef NDEBUG
		if(local_sum_temp == 0x80000000)
		{
			return -1;
		}
#endif
		*central_difference = 4*MATRIX_BSQ_INDEX(mySamples, x, y, z) - local_sum_temp;
	}

	return 0;
}


int Predictor::get_directional_difference(int directional_difference[3], unsigned short int * samples, unsigned int x, unsigned int y, unsigned int z)
{
	int local_sum_temp = local_sum(x, y, z, samples);

#ifndef NDEBUG
	if(local_sum_temp == 0x80000000)
	{
		return -1;
	}
	if(PredictionFull == 0)
	{
		fprintf(stderr, "Error: directional differences asked, but full prediction mode disabled.\n");
		return -1;
	}
#endif

	//full prediction mode, the differences in the directional_difference vector are
	//in this order: central, north, west, north-west
	if(y > 0)
	{
		directional_difference[0] = 4*MATRIX_BSQ_INDEX(mySamples, x, y - 1, z) - local_sum_temp;
		if(x > 0)
		{
			directional_difference[1] = 4*MATRIX_BSQ_INDEX(mySamples, x - 1, y, z) - local_sum_temp;
			directional_difference[2] = 4*MATRIX_BSQ_INDEX(mySamples, x - 1, y - 1, z) - local_sum_temp;
		}
		else
		{
			directional_difference[1] = directional_difference[0];
			directional_difference[2] = directional_difference[0];
		}
	}
	else
	{
		directional_difference[0] = 0;
		directional_difference[1] = 0;
		directional_difference[2] = 0;
	}

	return 0;
}

} /* namespace RiceAlgorithm */
