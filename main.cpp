#include "BMPProcessing.h"

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	if (argc < 2) {
		std::cout << "Usage: " << std::filesystem::path(argv[0]).filename().string() << " inputfile" << std::endl;
		return 1;
	}

	BMPReader bmpReader;
	bool ret = bmpReader.Open(argv[1]);
	if (!ret)
		return 2;

	bmpReader.PrintPicture();

	int32 key;
	std::cin >> key;

	return 0;
}

