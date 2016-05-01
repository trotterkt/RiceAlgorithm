/*
 * Reverse.h
 *
 *  Created on: Apr 19, 2016
 *      Author: trotterkt
 */

#ifndef REVERSE_H_
#define REVERSE_H_


unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


#endif /* REVERSE_H_ */
