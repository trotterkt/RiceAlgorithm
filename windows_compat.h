/*
 * windows_compat.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */


#ifndef WINDOWS_COMPAT_H_
#define WINDOWS_COMPAT_H_

// Allow for easier cross compile on Windows
#ifdef __MINGW32__
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif




#endif /* WINDOWS_COMPAT_H_ */
