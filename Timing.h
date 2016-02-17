/*
 * Timing.h
 *
 *  Created on: Feb 16, 2016
 *      Author: trottke1
 */

#ifndef TIMING_H_
#define TIMING_H_

#include <sys/time.h>

typedef unsigned long long timestamp_t;

const unsigned long MicroSecondsPerSecond(1000000);

static timestamp_t getTimestamp()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    return currentTime.tv_usec + (timestamp_t) currentTime.tv_sec * MicroSecondsPerSecond;
}

static double getSecondsDiff(timestamp_t t0, timestamp_t t1)
{ return (double(t1 - t0) / double(MicroSecondsPerSecond)); }


#endif /* TIMING_H_ */
