#include <iostream>
#include "DataExtractor.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " filename.wav" << std::endl;
		return 1;
	}

	const std::string inFile(argv[1]);
	DataExtractor extractor(inFile);
	extractor.run();

	std::cout << "ASCII representation" << std::endl << "=========" << std::endl;
	std::cout << extractor << std::endl << std::endl;

	extractor.setOutFormat(DataExtractor::E_OUT_FORMAT::BINARY);

	std::cout << "Message representation" << std::endl << "=========" << std::endl;
	std::cout << extractor << std::endl;

	return 0;
}
