/*
 * windows_compat.h
 *
 *  Created on: Feb 3, 2016
 *      Author: trottke1
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
