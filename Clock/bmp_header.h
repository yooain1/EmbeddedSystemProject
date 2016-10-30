#ifndef _BMPINFO_
#define _BMPINFO_

#define WIDTH 800
#define HEIGHT 480

typedef unsigned short U16;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned char BYTE;

typedef struct tagBITMAPFILEHEADER{
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
} __attribute((packed))BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biplanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} __attribute((packed)) BITMAPINFOHEADER;

#endif
	
