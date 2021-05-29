#pragma once

#include "common.h"

typedef int32 FXPT2DOT30;

struct CIEXYZ 
{
	FXPT2DOT30		ciexyzX;
	FXPT2DOT30		ciexyzY;
	FXPT2DOT30		ciexyzZ;
};

struct CIEXYZTriple
{
	CIEXYZ			ciexyzRed;
	CIEXYZ			ciexyzGreen;
	CIEXYZ			ciexyzBlue;
};

struct BMPFileHeader
{
	uint16			type;
	uint32			size;
	uint16			reserved1;
	uint16			reserved2;
	uint32			offBits;
};

struct BMPInfoHeader
{
	uint32			size;
	uint32			width;
	uint32			height;
	uint16			planes;
	uint16			bitCount;
	uint32			compression;
	uint32			sizeImage;
	uint32			XPelsPerMeter;
	uint32			YPelsPerMeter;
	uint32			clrUsed;
	uint32			clrImportant;
	uint32			redMask;
	uint32			greenMask;
	uint32			blueMask;
	uint32			alphaMask;
	uint32			CSType;
	CIEXYZTriple	endpoints;
	uint32			gammaRed;
	uint32			gammaGreen;
	uint32			gammaBlue;
	uint32			intent;
	uint32			profileData;
	uint32			profileSize;
	uint32			reserved;
};

struct RGBQuad
{
	uint8			rgbBlue;
	uint8			rgbGreen;
	uint8			rgbRed;
	uint8			rgbReserved;
	uint32			combined;
};

class BMPReader
{
public:
	~BMPReader() 
	{
		if (fileStream.is_open())
			fileStream.close();

		for (uint32 i = 0; i < fileInfoHeader.height; i++) {
			delete[] rgbInfo[i];
		}

		delete[] rgbInfo;
	}

	bool Open(const std::string& inf);
	void PrintInfo();
	void PrintPicture();

private:
	bool ReadFileHeader();
	bool ReadFileInfoHeader();

	template <typename T>
	void Read(std::ifstream& fp, T& result, std::size_t size)
	{
		fp.read(reinterpret_cast<char*>(&result), size);
	}

	uint8 BitExtract(const uint32 byte, const uint32 mask);
	std::string VersionToString();

private:
	std::string		fileName;
	std::ifstream	fileStream;
	BMPFileHeader	fileHeader;
	BMPInfoHeader	fileInfoHeader;
	RGBQuad**		rgbInfo;
};



