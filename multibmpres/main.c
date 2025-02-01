#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __crt_malloc malloc
#define __crt_realloc realloc
#define __crt_strcmp strcmp
#define __crt_memcpy memcpy
#define __crt_strcpy strcpy
#define __crt_memset memset

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
char* MultiBmpClient_GetResName(void* _ResList, int i);
unsigned long MultiBmpClient_GetResSize(void* _ResList, int i, unsigned long* Width, unsigned long* Height);
unsigned char* MultiBmpClient_GetBytesFromBMP(unsigned char* fileData, int* width, int* height, int* bytesPerPixel, unsigned long* RawDataSize);


int main(int argc, char** argv) {
	if (argc == 1) {
		// interactive mode

		FILE* File = fopen("res.bin", "rb");
		if (!File) {
			File = fopen("res.bin", "wb");
			
			RESOURCE_LIST ResList = { 0 };
			ResList.FirstRes = 0x4;
			fwrite(&ResList, sizeof(RESOURCE_LIST), 1, File);
			RESOURCE_ITEM FirstItem = { 0 };
			fwrite(&FirstItem, sizeof(RESOURCE_ITEM), 1, File);
			fclose(File);
		}

		File = fopen("res.bin", "rb+");
		if (!File) {
			printf("Failed to open file.\n");
			return -1;
		}

		unsigned long ResListSize;
		fseek(File, 0, SEEK_END);
		ResListSize = ftell(File);
		fseek(File, 0, SEEK_SET);

		void* _ResList = malloc(ResListSize);
		fread(_ResList, ResListSize, 1, File);

		printf("MultiBMPRes Tool v1.00\n");
		printf("(c) Noah Wooten, All Rights Reserved 2023-2025\n\n");

		while (1) {
			printf("1.) Add new resource\n");
			printf("2.) List resources\n");
			printf("3.) Exit\n");

			char Inbuffer[64];
			fgets(Inbuffer, 64, stdin);
			int Selector = atoi(Inbuffer);

			switch (Selector) {
			case 1: // add new resource
				printf("Resource file name: ");
				fgets(Inbuffer, 128, stdin);
				if (strstr(Inbuffer, "\n"))
					strstr(Inbuffer, "\n")[0] = 0x00;

				FILE* _Open = fopen(Inbuffer, "rb");
				if (!_Open) {
					printf("Failed to open destination file.\n");
					return 0;
				}
				
				unsigned long OpenData;
				fseek(_Open, 0, SEEK_END);
				OpenData = ftell(_Open);
				fseek(_Open, 0, SEEK_SET);

				void* Data = malloc(OpenData);
				fread(Data, OpenData, 1, _Open);
				fclose(_Open);

				printf("Resource name: ");
					fgets(Inbuffer, 128, stdin);
				if (strstr(Inbuffer, "\n"))
					strstr(Inbuffer, "\n")[0] = 0x00;

				unsigned long OldResListSize = ResListSize;
				_ResList = MultiBmpClient_AddResWithBmpData(_ResList, &ResListSize, Inbuffer, Data);
				free(Data);

				printf("Added resource '%s', added %i KiB. (%i%% compression)\n", Inbuffer, (ResListSize - OldResListSize) / 1024, 100 - (((ResListSize - OldResListSize) / OpenData) * 100));
				break;
			case 2: // list resources
				int ResCnt = MultiBmpClient_GetResCount(_ResList);
				for (int i = 0; i < ResCnt; i++) {
					unsigned long Width, Height;
					unsigned long Compressed = MultiBmpClient_GetResSize(_ResList, i, &Width, &Height) / 1024;
					unsigned long FullSize = (Width * Height * 3) / 1024;
					printf("%i.) '%s' (%ix%i, %i KiB, %i KiB Uncompressed)\n", i + 1, MultiBmpClient_GetResName(_ResList, i),
						Width, Height, Compressed, FullSize);
				}
				break;
			default: // exit
			case 3:
				fseek(File, 0, SEEK_SET);
				fwrite(_ResList, ResListSize, 1, File);
				printf("Wrote %i KiB.\n", ResListSize / 1024);
				fclose(File);
				return 0;
				break;
			}

		}
	} else {
		// scripting mode
	}

	return 0;
}

int MultiBmpClient_GetResCount(void* _ResList) {
	PRESOURCE_LIST ResList = _ResList;
	unsigned long Count = 0;
	PRESOURCE_ITEM Item = ((char*)_ResList) + ResList->FirstRes;
	
	do {
		Count++;
		if (Item->NextResource && Item->NextResource != 0xFFFFFFFF)
			Item = (char*)_ResList + Item->NextResource;
		else
			break;
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
		if (Item->NextResource)
			Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return NULL;
}

void* MultiBmpClient_AddResWithBmpData(void* _ResList, unsigned long* ResListSize, char* Name, void* BmpFileData) {
	PRESOURCE_LIST ResList = _ResList;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	unsigned long OldNext = ResList->FirstRes;

	do {
		if (!Item->RawDataLocation) {
			unsigned long Width, Height, BPP, RawSize;
			void* BitmapData = MultiBmpClient_GetBytesFromBMP(BmpFileData, &Width, &Height, &BPP, &RawSize);

			_ResList = __crt_realloc(_ResList, *ResListSize + RawSize);
			__crt_memcpy((char*)_ResList + *ResListSize, BitmapData, RawSize);
			PRESOURCE_ITEM NewItem = (char*)_ResList + OldNext;

			NewItem->Width = Width;
			NewItem->Height = Height;
			NewItem->RawDataLocation = *ResListSize;
			NewItem->RawDataSize = RawSize;
			__crt_strcpy(NewItem->ResName, Name);
			*ResListSize += RawSize;
			NewItem->NextResource = *ResListSize;
			
			_ResList = __crt_realloc(_ResList, *ResListSize + sizeof(RESOURCE_ITEM));
			__crt_memset((char*)_ResList + *ResListSize, 0, sizeof(RESOURCE_ITEM));

			*ResListSize += sizeof(RESOURCE_ITEM);

			return _ResList;
		} else {
			OldNext = Item->NextResource;
			Item = (char*)_ResList + Item->NextResource;
		}
	} while (1);
}

char* MultiBmpClient_GetResName(void* _ResList, int i) {
	PRESOURCE_LIST ResList = _ResList;
	unsigned long Count = 0;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	do {
		if (Count == i) {
			return Item->ResName;
		}
		Count++;
		Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return NULL;
}

unsigned long MultiBmpClient_GetResSize(void* _ResList, int i, unsigned long* Width, unsigned long* Height) {
	PRESOURCE_LIST ResList = _ResList;
	unsigned long Count = 0;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	do {
		if (Count == i) {
			*Width = Item->Width;
			*Height = Item->Height;
			return Item->RawDataSize;
		}
		Count++;
		Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return NULL;
}