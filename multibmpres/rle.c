#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define __crt_malloc malloc
#define __crt_realloc realloc
#define __crt_free free
#define __crt_strcmp strcmp
#define __crt_memcpy memcpy

void* MultiBmpClient_Compress(void* RawIn, unsigned long RawSize, unsigned long* OutSize) {
    if (!RawIn || RawSize == 0) {
        *OutSize = 0;
        return NULL;
    }

    unsigned char* Input = (unsigned char*)RawIn;
    // Allocate worst-case output buffer (each input byte becomes 2 bytes)
    unsigned long MaxOutputSize = RawSize * 2;
    unsigned char* Return = __crt_malloc(MaxOutputSize);
    if (!Return) {
        *OutSize = 0;
        return NULL;
    }

    unsigned long InIndex = 0;
    unsigned long OutIndex = 0;

    while (InIndex < RawSize) {
        unsigned char Current = Input[InIndex];
        unsigned int RunLength = 1;

        // Count how many consecutive bytes are equal to 'current'
        // Limit the run length to 255 (so it fits in one byte)
        while ((InIndex + RunLength < RawSize) &&
            (Input[InIndex + RunLength] == Current) &&
            (RunLength < 255)) {
            RunLength++;
        }

        // Write the run length and the byte value to the output
        Return[OutIndex++] = (unsigned char)RunLength;
        Return[OutIndex++] = Current;

        // Move past this run in the input data
        InIndex += RunLength;
    }

    *OutSize = OutIndex;

    // Optionally, shrink the allocated buffer to the actual output size
    unsigned char* ShrunkOutput = __crt_realloc(Return, OutIndex);
    return ShrunkOutput ? ShrunkOutput : Return;
}

void* MultiBmpClient_Decompress(void* CompIn, unsigned long CompSize, unsigned long* UnCompSize) {
    if (!CompIn || CompSize == 0) {
        *UnCompSize = 0;
        return NULL;
    }

    unsigned char* Comp = (unsigned char*)CompIn;
    unsigned long InIndex = 0;
    unsigned long TotalOutputSize = 0;

    // First pass: calculate the size of the uncompressed data.
    while (InIndex < CompSize) {
        // Ensure that we have a full pair available.
        if (InIndex + 1 >= CompSize) {
            // Corrupted/compressed data: not enough bytes for a complete pair.
            *UnCompSize = 0;
            return NULL;
        }

        unsigned char Count = Comp[InIndex];
        TotalOutputSize += Count;
        InIndex += 2;
    }

    // Allocate the output buffer
    unsigned char* Output = __crt_malloc(TotalOutputSize);
    
    if (!Output) {
        *UnCompSize = 0;
        return NULL;
    }

    InIndex = 0;
    unsigned long OutIndex = 0;

    // Second pass: decompress the data.
    while (InIndex < CompSize) {
        unsigned char count = Comp[InIndex++];
        unsigned char value = Comp[InIndex++];

        // Write 'count' copies of 'value'
        for (unsigned int i = 0; i < count; i++) {
            Output[OutIndex++] = value;
        }
    }

    *UnCompSize = TotalOutputSize;
    return Output;
}