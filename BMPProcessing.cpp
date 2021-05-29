#include "BMPProcessing.h"

enum BMPVersions
{
	BMP_VERSION_0 = 12,
	BMP_VERSION_1 = 40,
	BMP_VERSION_2 = 52,
	BMP_VERSION_3 = 56,
	BMP_VERSION_4 = 108,
	BMP_VERSION_5 = 124,
};

enum BMPSupportedBits
{
	BIT_16 = 16,
	BIT_24 = 24,
	BIT_32 = 32
};

enum BMPCompressions
{
	BI_RGB,
	BI_RLE8,
	BI_RLE4,
	BI_BITFIELDS,
	BI_JPEG,
	BI_PNG,
	BI_ALPHABITFIELDS
};

bool BMPReader::Open(const std::string& inf)
{
	fileName = inf;
	fileStream.open(inf, std::ifstream::binary);
	if (!fileStream) {
		std::cout << "Error opening file '" << inf << "'." << std::endl;
		return false;
	}

	if (!ReadFileHeader()) {
		return false;
	}
	if (!ReadFileInfoHeader()) {
		return false;
	}


	return true;
}

bool BMPReader::ReadFileHeader()
{
	Read(fileStream, fileHeader.type, sizeof(fileHeader.type));
	Read(fileStream, fileHeader.size, sizeof(fileHeader.size));
	Read(fileStream, fileHeader.reserved1, sizeof(fileHeader.reserved1));
	Read(fileStream, fileHeader.reserved2, sizeof(fileHeader.reserved2));
	Read(fileStream, fileHeader.offBits, sizeof(fileHeader.offBits));

	if (fileHeader.type != 0x4D42) {
		std::cout << "Error: '" << fileName << "' is not BMP file." << std::endl;
		return false;
	}

	return true;
}

bool BMPReader::ReadFileInfoHeader()
{
	Read(fileStream, fileInfoHeader.size, sizeof(fileInfoHeader.size));

	if (fileInfoHeader.size >= BMP_VERSION_0) {
		Read(fileStream, fileInfoHeader.width, sizeof(fileInfoHeader.width));
		Read(fileStream, fileInfoHeader.height, sizeof(fileInfoHeader.height));
		Read(fileStream, fileInfoHeader.planes, sizeof(fileInfoHeader.planes));
		Read(fileStream, fileInfoHeader.bitCount, sizeof(fileInfoHeader.bitCount));
	}

	int32 colorsCount = fileInfoHeader.bitCount >> 3;
	if (colorsCount < 3) {
		colorsCount = 3;
	}

	int32 bitsOnColor = fileInfoHeader.bitCount / colorsCount;
	int32 maskValue = (1 << bitsOnColor) - 1;

	if (fileInfoHeader.size >= BMP_VERSION_1) {
		Read(fileStream, fileInfoHeader.compression, sizeof(fileInfoHeader.compression));
		Read(fileStream, fileInfoHeader.sizeImage, sizeof(fileInfoHeader.sizeImage));
		Read(fileStream, fileInfoHeader.XPelsPerMeter, sizeof(fileInfoHeader.XPelsPerMeter));
		Read(fileStream, fileInfoHeader.YPelsPerMeter, sizeof(fileInfoHeader.YPelsPerMeter));
		Read(fileStream, fileInfoHeader.clrUsed, sizeof(fileInfoHeader.clrUsed));
		Read(fileStream, fileInfoHeader.clrImportant, sizeof(fileInfoHeader.clrImportant));
	}

	fileInfoHeader.redMask = 0;
	fileInfoHeader.greenMask = 0;
	fileInfoHeader.blueMask = 0;

	if (fileInfoHeader.size >= BMP_VERSION_2) {
		Read(fileStream, fileInfoHeader.redMask, sizeof(fileInfoHeader.redMask));
		Read(fileStream, fileInfoHeader.greenMask, sizeof(fileInfoHeader.greenMask));
		Read(fileStream, fileInfoHeader.blueMask, sizeof(fileInfoHeader.blueMask));
	}

	if (fileInfoHeader.redMask == 0 || fileInfoHeader.greenMask == 0 || fileInfoHeader.blueMask == 0) {
		fileInfoHeader.redMask = maskValue << (bitsOnColor * 2);
		fileInfoHeader.greenMask = maskValue << bitsOnColor;
		fileInfoHeader.blueMask = maskValue;
	}

	if (fileInfoHeader.size >= BMP_VERSION_3) {
		Read(fileStream, fileInfoHeader.alphaMask, sizeof(fileInfoHeader.alphaMask));
	}
	else {
		fileInfoHeader.alphaMask = maskValue << (bitsOnColor * 3);
	}

	if (fileInfoHeader.size >= BMP_VERSION_4) {
		Read(fileStream, fileInfoHeader.CSType, sizeof(fileInfoHeader.CSType));
		Read(fileStream, fileInfoHeader.endpoints, sizeof(fileInfoHeader.endpoints));
		Read(fileStream, fileInfoHeader.gammaRed, sizeof(fileInfoHeader.gammaRed));
		Read(fileStream, fileInfoHeader.gammaGreen, sizeof(fileInfoHeader.gammaGreen));
		Read(fileStream, fileInfoHeader.gammaBlue, sizeof(fileInfoHeader.gammaBlue));
	}

	if (fileInfoHeader.size >= BMP_VERSION_5) {
		Read(fileStream, fileInfoHeader.intent, sizeof(fileInfoHeader.intent));
		Read(fileStream, fileInfoHeader.profileData, sizeof(fileInfoHeader.profileData));
		Read(fileStream, fileInfoHeader.profileSize, sizeof(fileInfoHeader.profileSize));
		Read(fileStream, fileInfoHeader.reserved, sizeof(fileInfoHeader.reserved));
	}

	if (fileInfoHeader.size != BMP_VERSION_0 && fileInfoHeader.size != BMP_VERSION_1 && fileInfoHeader.size != BMP_VERSION_2 &&
		fileInfoHeader.size != BMP_VERSION_3 && fileInfoHeader.size != BMP_VERSION_4 && fileInfoHeader.size != BMP_VERSION_5) {
		std::cout << "Error: Unsupported BMP format." << std::endl;
		return false;
	}

	if (fileInfoHeader.bitCount != BIT_16 && fileInfoHeader.bitCount != BIT_24 && fileInfoHeader.bitCount != BIT_32) {
		std::cout << "Error: Unsupported BMP bit count." << std::endl;
		return false;
	}

	if (fileInfoHeader.compression != BI_RGB && fileInfoHeader.compression != BI_BITFIELDS) {
		std::cout << "Error: Unsupported BMP compression." << std::endl;
		return false;
	}

	int32 linePadding = ((fileInfoHeader.width * (fileInfoHeader.bitCount / 8)) % 4) & 3;

	rgbInfo = new RGBQuad* [fileInfoHeader.height];

	for (uint32 i = 0; i < fileInfoHeader.height; i++) {
		rgbInfo[i] = new RGBQuad[fileInfoHeader.width];
	}

	uint32 buf;

	for (uint32 i = 0; i < fileInfoHeader.height; i++) {
		for (uint32 j = 0; j < fileInfoHeader.width; j++) {
			Read(fileStream, buf, fileInfoHeader.bitCount / 8);

			rgbInfo[i][j].rgbRed = BitExtract(buf, fileInfoHeader.redMask);
			rgbInfo[i][j].rgbGreen = BitExtract(buf, fileInfoHeader.greenMask);
			rgbInfo[i][j].rgbBlue = BitExtract(buf, fileInfoHeader.blueMask);
			rgbInfo[i][j].rgbReserved = BitExtract(buf, fileInfoHeader.alphaMask);
			rgbInfo[i][j].combined = rgbInfo[i][j].rgbRed << 16 | rgbInfo[i][j].rgbGreen << 8 | rgbInfo[i][j].rgbBlue;
		}
		fileStream.seekg(linePadding, std::ios_base::cur);
	}

	return true;
}

uint8 BMPReader::BitExtract(const uint32 byte, const uint32 mask)
{
	if (mask == 0) {
		return 0;
	}

	int32 maskBufer = mask, maskPadding = 0;

	while (!(maskBufer & 1)) {
		maskBufer >>= 1;
		maskPadding++;
	}

	return (byte & mask) >> maskPadding;
}

void BMPReader::PrintInfo()
{
	for (uint32 i = 0; i < fileInfoHeader.height; i++) {
		for (uint32 j = 0; j < fileInfoHeader.width; j++) {
			std::cout << std::hex << rgbInfo[i][j].combined << std::endl;
		}
		std::cout << std::endl;
	}

	std::cout << std::dec;
	std::cout << "Width = " << fileInfoHeader.width << std::endl;
	std::cout << "Height = " << fileInfoHeader.height << std::endl;
	std::cout << "Bits = " << fileInfoHeader.bitCount << std::endl;
	std::cout << "Version = " << VersionToString() << std::endl;

	std::cout << std::endl;
}

std::string BMPReader::VersionToString()
{
	switch (fileInfoHeader.size) {
	case BMP_VERSION_0: return "Core";
	case BMP_VERSION_1: return "1";
	case BMP_VERSION_2: return "2";
	case BMP_VERSION_3: return "3";
	case BMP_VERSION_4: return "4";
	case BMP_VERSION_5: return "5";
	}

	return "Unknown";
}

void BMPReader::PrintPicture()
{
	for (uint32 i = 0; i < fileInfoHeader.height; i++) {
		for (uint32 j = 0; j < fileInfoHeader.width; j++) {
			if(!rgbInfo[i][j].combined) 
				std::cout << "@";
			else
				std::cout << " ";
		}
		std::cout << std::endl;
	}
}

