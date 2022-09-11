#include <iostream>

int main(int argc, char* argv[])
{
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			std::cout << argv[i] << std::endl;
		}
	}
	std::cout << "Stack size" << std::endl;
}
