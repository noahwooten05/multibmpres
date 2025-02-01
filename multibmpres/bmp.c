#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __crt_memcpy memcpy
#define __crt_malloc malloc

// BMP file header structure
#pragma pack(push, 1) // Ensure no padding
typedef struct {
    unsigned short bfType;      // File type, must be 'BM'
    unsigned int bfSize;        // Size of the file (in bytes)
    unsigned short bfReserved1; // Reserved, must be 0
    unsigned short bfReserved2; // Reserved, must be 0
    unsigned int bfOffBits;     // Offset to start of pixel data
} BMPFileHeader;

// BMP info header structure
typedef struct {
    unsigned int biSize;          // Size of this header (40 bytes)
    int biWidth;                  // Width of the image
    int biHeight;                 // Height of the image
    unsigned short biPlanes;      // Number of color planes (must be 1)
    unsigned short biBitCount;    // Bits per pixel (24 for RGB)
    unsigned int biCompression;   // Compression type (0 = none)
    unsigned int biSizeImage;     // Image size (can be 0 if uncompressed)
    int biXPelsPerMeter;          // Horizontal resolution
    int biYPelsPerMeter;          // Vertical resolution
    unsigned int biClrUsed;       // Number of colors used (0 = all colors)
    unsigned int biClrImportant;  // Number of important colors (0 = all)
} BMPInfoHeader;
#pragma pack(pop)

// Function to load BMP file from memory into a byte array
unsigned char* MultiBmpClient_GetBytesFromBMP(unsigned char* fileData, int* width, int* height, int* bytesPerPixel) {
    const unsigned char* dataPtr = fileData;

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    // Read the file header
    __crt_memcpy(&fileHeader, dataPtr, sizeof(BMPFileHeader));
    dataPtr += sizeof(BMPFileHeader);

    if (fileHeader.bfType != 0x4D42) { // 'BM' in little-endian
        return NULL;
    }

    // Read the info header
    __crt_memcpy(&infoHeader, dataPtr, sizeof(BMPInfoHeader));
    dataPtr += sizeof(BMPInfoHeader);

    *width = infoHeader.biWidth;
    *height = infoHeader.biHeight;
    *bytesPerPixel = infoHeader.biBitCount / 8;

    if (infoHeader.biCompression != 0 || *bytesPerPixel != 3) {
        return NULL;
    }

    // Allocate memory for pixel data
    unsigned char* pixelData = (unsigned char*)__crt_malloc(infoHeader.biSizeImage);
    if (!pixelData) {
        return NULL;
    }

    // Copy pixel data from memory
    __crt_memcpy(pixelData, fileData + fileHeader.bfOffBits, infoHeader.biSizeImage);

    return pixelData;
}