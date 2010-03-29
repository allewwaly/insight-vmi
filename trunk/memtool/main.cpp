
#include <iostream>
#include "char.h"

int main(int argc, char* argv[])
{
    std::cout << "Hello, world! My arguments are:" << std::endl;

    for (int i = 0; i < argc; i++)
        std::cout << i << ": " << argv[i] << std::endl;

    Char c(1, 1, 0);

    return 0;
}
