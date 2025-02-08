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
#define __crt_free free

typedef struct _RESOURCE_ITEM {
	char ResName[256];

	unsigned long RawDataLocation;
	unsigned long RawDataSize;
	unsigned long Width, Height;
	unsigned char NoCompress;
	
	unsigned long NextResource;
}RESOURCE_ITEM, *PRESOURCE_ITEM;

typedef struct _RESOURCE_LIST {
	unsigned long FirstRes;
}RESOURCE_LIST, *PRESOURCE_LIST;

int   MultiBmpClient_GetResCount(void* ResList);
int   MultiBmpClient_GetResByName(void* ResList, char* Name);
void* MultiBmpClient_GetBmpDataFromRes(void* ResList, int i, unsigned short* Width, unsigned short* Height);
void* MultiBmpClient_AddResWithBmpData(void* ResList, unsigned long* ResListSize, char* Name, void* BmpFileData, unsigned char NoCompress, unsigned char NoFlip);
void* MultiBmpClient_Compress(void* RawIn, unsigned long RawSize, unsigned long* OutSize);
void* MultiBmpClient_Decompress(void* CompIn, unsigned long CompSize, unsigned long* UnCompSize);
char* MultiBmpClient_GetResName(void* _ResList, int i);
unsigned long MultiBmpClient_GetResSize(void* _ResList, int i, unsigned long* Width, unsigned long* Height);
unsigned char* MultiBmpClient_GetBytesFromBMP(unsigned char* fileData, int* width, int* height, int* bytesPerPixel, unsigned long* RawDataSize);

void FlipBitmapVertically(void* Data, unsigned long Width, unsigned long Height) {
	if (!Data) return;

	unsigned long rowSize = Width * 3;  // Each row is Width * 3 bytes
	unsigned char* buffer = (unsigned char*)malloc(rowSize);
	if (!buffer) return;

	unsigned char* pixels = (unsigned char*)Data;
	for (unsigned long y = 0; y < Height / 2; y++) {
		unsigned char* rowTop = pixels + (y * rowSize);
		unsigned char* rowBottom = pixels + ((Height - 1 - y) * rowSize);

		// Swap the two rows
		memcpy(buffer, rowTop, rowSize);
		memcpy(rowTop, rowBottom, rowSize);
		memcpy(rowBottom, buffer, rowSize);
	}

	free(buffer);
}

int main(int argc, char** argv) {
	//fgetc(stdin);

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
				_ResList = MultiBmpClient_AddResWithBmpData(_ResList, &ResListSize, Inbuffer, Data, 0, 1);
				int GetName = MultiBmpClient_GetResByName(_ResList, Inbuffer);
				unsigned long MD_Width, MD_Height;
				MultiBmpClient_GetResSize(_ResList, GetName, &MD_Width, &MD_Height);
				free(Data);

				double compressionPercentage = (1.0 - ((double)(ResListSize - OldResListSize) / (3 * MD_Width * MD_Height))) * 100.0;
 				printf("Added resource '%s', added %i KiB. (%0.2f%% compression)\n", Inbuffer, (ResListSize - OldResListSize) / 1024, compressionPercentage);
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

		printf("MultiBMPRes Tool v1.00\n");
		printf("(c) Noah Wooten, All Rights Reserved 2023-2025\n\n");
		
		if (!strcmp(argv[1], "--help")) {
Help:
			printf("multibmpres.exe --help: Displays this screen\n");
			printf("multibmpres.exe file_store.res --add (--no-compress) (--no-flip) image0.bmp Image0 : Adds an image\n");
			printf("multibmpres.exe file_store.res --list : Lists all images\n");
			
			return 0;
		}

		FILE* File = fopen(argv[1], "rb");
		if (!File) {
			File = fopen(argv[1], "wb");
			RESOURCE_LIST ResList = { 0 };
			ResList.FirstRes = 0x4;
			fwrite(&ResList, sizeof(RESOURCE_LIST), 1, File);
			RESOURCE_ITEM FirstItem = { 0 };
			fwrite(&FirstItem, sizeof(RESOURCE_ITEM), 1, File);
		}

		fclose(File);
		File = fopen(argv[1], "rb+");

		if (!strcmp(argv[2], "--add")) {
			char* FileName = argv[3], *ResName = argv[4];
			unsigned char NoCompress = 0, NoFlip = 0;

			if (!strcmp(argv[3], "--no-compress")) {
				if (!strcmp(argv[4], "--no-flip")) {
					FileName = argv[5];
					ResName = argv[6];
					NoFlip = 1;
				} else {
					FileName = argv[4];
					ResName = argv[5];
				}
				NoCompress = 1;
			} else {
				if (!strcmp(argv[3], "--no-flip")) {
					FileName = argv[4];
					ResName = argv[5];
					NoFlip = 1;
				}
			}

			if (!FileName || !ResName) {
				printf("Missing argument.\n");
				return 0;
			}

			unsigned long StoreSize;
			fseek(File, 0, SEEK_END);
			StoreSize = ftell(File);
			fseek(File, 0, SEEK_SET);
			void* _ResFile = malloc(StoreSize);
			fread(_ResFile, StoreSize, 1, File);

			FILE* Bitmap = fopen(FileName, "rb");
			if (!Bitmap) {
				printf("Failed to open bitmap file.\n");
				return 0;
			}

			unsigned long BMPSize;
			fseek(Bitmap, 0, SEEK_END);
			BMPSize = ftell(Bitmap);
			fseek(Bitmap, 0, SEEK_SET);
			void* BitmapData = malloc(BMPSize);
			fread(BitmapData, BMPSize, 1, Bitmap);
			fclose(Bitmap);

			unsigned long OldStoreSize = StoreSize;
			_ResFile = MultiBmpClient_AddResWithBmpData(_ResFile, &StoreSize, ResName, BitmapData, NoCompress, NoFlip);
			free(BitmapData);

			double compressionPercentage = (1.0 - ((double)(StoreSize - OldStoreSize) / (BMPSize))) * 100.0;
			printf("Added resource '%s', added %i KiB (%0.2f%% compression)\n", ResName, (StoreSize - OldStoreSize) / 1024, compressionPercentage);
			
			fseek(File, 0, SEEK_SET);
			fwrite(_ResFile, StoreSize, 1, File);
			fflush(File);
			fclose(File);
			free(_ResFile);
			return 0;

		} else if (!strcmp(argv[2], "--list")) {
			unsigned long StoreSize;
			fseek(File, 0, SEEK_END);
			StoreSize = ftell(File);
			fseek(File, 0, SEEK_SET);
			void* _ResFile = malloc(StoreSize);
			fread(_ResFile, StoreSize, 1, File);

			int ResCnt = MultiBmpClient_GetResCount(_ResFile);
			for (int i = 0; i < ResCnt; i++) {
				unsigned long Width, Height;
				unsigned long Compressed = MultiBmpClient_GetResSize(_ResFile, i, &Width, &Height) / 1024;
				unsigned long FullSize = (Width * Height * 3) / 1024;
				printf("%i.) '%s' (%ix%i, %i KiB, %i KiB Uncompressed)\n", i + 1, MultiBmpClient_GetResName(_ResFile, i),
					Width, Height, Compressed, FullSize);
			}

			free(_ResFile);
			fclose(File);

			return 0;
		} else {
			goto Help;
		}
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
			unsigned long OutSize;
			void* _Return = MultiBmpClient_Decompress(Return, Item->RawDataSize, &OutSize);
			*Width = Item->Width;
			*Height = Item->Height;
			__crt_free(Return);
			return _Return;
		}

		Count++;
		if (Item->NextResource)
			Item = (char*)_ResList + Item->NextResource;
	} while (Item->NextResource);

	return NULL;
}

void* MultiBmpClient_AddResWithBmpData(void* _ResList, unsigned long* ResListSize, char* Name, void* BmpFileData, unsigned char NoCompress, unsigned char NoFlip) {
	PRESOURCE_LIST ResList = _ResList;
	PRESOURCE_ITEM Item = (char*)_ResList + ResList->FirstRes;

	unsigned long OldNext = ResList->FirstRes;

	do {
		if (!Item->RawDataLocation) {
			unsigned long Width, Height, BPP, RawSize;
			void* BitmapData = MultiBmpClient_GetBytesFromBMP(BmpFileData, &Width, &Height, &BPP, &RawSize);
			if (!NoFlip)
				FlipBitmapVertically(BitmapData, Width, Height);

			unsigned long _RawSize;
			void* _BitmapData = NULL;
			if (!NoCompress)
				_BitmapData = MultiBmpClient_Compress(BitmapData, RawSize, &_RawSize);
			else {
				_BitmapData = malloc(RawSize);
				_RawSize = RawSize;
				memcpy(_BitmapData, BitmapData, RawSize);
			}

			_ResList = __crt_realloc(_ResList, *ResListSize + _RawSize);
			__crt_free(BitmapData);
			__crt_memcpy((char*)_ResList + *ResListSize, _BitmapData, _RawSize);
			PRESOURCE_ITEM NewItem = (char*)_ResList + OldNext;

			NewItem->Width = Width;
			NewItem->Height = Height;
			NewItem->RawDataLocation = *ResListSize;
			NewItem->RawDataSize = _RawSize;
			NewItem->NoCompress = NoCompress;
			__crt_strcpy(NewItem->ResName, Name);
			*ResListSize += _RawSize;
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