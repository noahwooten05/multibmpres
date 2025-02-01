#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __crt_malloc malloc
#define __crt_realloc realloc
#define __crt_strcmp strcmp
#define __crt_memcpy memcpy
#define __crt_strcpy strcpy

typedef struct _RESOURCE_ITEM {
	char ResName[256];

	unsigned long RawDataLocation;
	unsigned long RawDataSize;
	unsigned long Width, Height;
	
	unsigned long NextResource;
}RESOURCE_ITEM, *PRESOURCE_ITEM;

typedef struct _RESOURCE_LIST {
	unsigned long FirstRes;
}RESOURCE_LIST, *PRESOURCE_LIST;

int   MultiBmpClient_GetResCount(void* ResList);
int   MultiBmpClient_GetResByName(void* ResList, char* Name);
void* MultiBmpClient_GetBmpDataFromRes(void* ResList, int i, unsigned short* Width, unsigned short* Height);
void* MultiBmpClient_AddResWithBmpData(void* ResList, unsigned long* ResListSize, char* Name, void* BmpFileData);
void* MultiBmpClient_Compress(void* RawIn, unsigned long RawSize, unsigned long* OutSize);
void* MultiBmpClient_Decompress(void* CompIn, unsigned long CompSize, unsigned long* UnCompSize);
unsigned char* MultiBmpClient_GetBytesFromBMP(unsigned char* fileData, int* width, int* height, int* bytesPerPixel, unsigned long* RawDataSize);

int main(int argc, char** argv) {
	return 0;
}

int MultiBmpClient_GetResCount(void* _ResList) {
	PRESOURCE_LIST ResList = _ResList;
	unsigned long Count = 0;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;
	
	do {
		Count++;
		Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return Count;
}

int MultiBmpClient_GetResByName(void* _ResList, char* Name) {
	PRESOURCE_LIST ResList = _ResList;
	unsigned long Count = 0;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	do {
		if (!__crt_strcmp(Item->ResName, Name))
			return Count;
		Count++;
		Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return -1;
}

void* MultiBmpClient_GetBmpDataFromRes(void* _ResList, int i, unsigned short* Width, unsigned short* Height) {
	PRESOURCE_LIST ResList = _ResList;
	unsigned long Count = 0;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	do {
		if (Count == i) {
			// get this data
			void* Return = __crt_malloc(Item->RawDataSize);
			__crt_memcpy(Return, (char*)_ResList + Item->RawDataLocation, Item->RawDataSize);
			*Width = Item->Width;
			*Height = Item->Height;
		}

		Count++;
		Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return NULL;
}

void* MultiBmpClient_AddResWithBmpData(void* _ResList, unsigned long* ResListSize, char* Name, void* BmpFileData) {
	PRESOURCE_LIST ResList = _ResList;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	do {
		if (!Item->NextResource) {
			_ResList = __crt_realloc(_ResList, *ResListSize + sizeof(RESOURCE_ITEM));
			PRESOURCE_ITEM NewItem = (char*)_ResList + *ResListSize;
			*ResListSize += sizeof(RESOURCE_ITEM);

			unsigned long Width, Height, BPP, RawSize;
			void* BitmapData = MultiBmpClient_GetBytesFromBMP(BmpFileData, &Width, &Height, &BPP, &RawSize);

			_ResList = __crt_realloc(_ResList, *ResListSize + RawSize);
			__crt_memcpy((char*)_ResList + *ResListSize, BitmapData, RawSize);

			NewItem->Width = Width;
			NewItem->Height = Height;
			NewItem->RawDataLocation = *ResListSize;
			NewItem->RawDataSize = RawSize;
			__crt_strcpy(NewItem->ResName, Name);
			NewItem->NextResource = 0;

			*ResListSize += RawSize;
		}

		Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);
}