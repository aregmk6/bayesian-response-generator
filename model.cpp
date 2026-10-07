#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
    std::ifstream matrix_data{
        "/home/aregmk/Coding/twopairencoding/trained_data.bin",
        std::ios::binary};
    if (!matrix_data.is_open()) {
        std::cerr << "file failed to open" << std::endl;
        exit(1);
    }

    size_t vocab_size = 0;

    matrix_data.read(reinterpret_cast<char*>(&vocab_size), sizeof(vocab_size));
    if (vocab_size == 0) {
        std::cerr << "vocab size if zero" << std::endl;
        exit(1);
    }

    std::cout << "Vocab size is: " << vocab_size << std::endl;

    return 0;
}
