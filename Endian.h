/*
 * Endian.h
 *
 *  Created by: Keir Trotter
 *  California State University, Fullerton
 *  MSE, CPSC 597, Graduate Project
 *
 *  Copyright 2016 Keir Trotter
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <stdint.h>

inline static bool isSystemBigEndian()
{
    int endianCheck = 1;

    return ((*(reinterpret_cast<char*>(&endianCheck)) == 0));
}

template <typename T>
inline void bigEndianVersusLittleEndian(T &numberToTranslate)
{
    const size_t bytes = sizeof(T);

    if(!isSystemBigEndian())
    {
        uint8_t buffer[bytes];
        memcpy(buffer, &numberToTranslate, bytes);

        int i = 0;
        int j = bytes -1;

        while(i<j)
        {
            std::swap(buffer[i], buffer[j]);
            i++;
            j--;
        }
        memcpy(&numberToTranslate, buffer, bytes);
    }
}




#endif /* ENDIAN_H_ */
