#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Token = size_t;

static std::vector<std::vector<size_t>> token_matrix{};
static std::vector<std::string> vocab{};

std::string stringfy(const std::vector<Token>& tokenized_str)
{
    std::stringstream output{};
    for (const auto& tok : tokenized_str) {
        output << vocab[tok];
    }

    return output.str();
}

Token predict_next_token(Token prev_tok)
{
}

std::string generate_sentence(const char* input)
{
    std::vector<Token> tstr{};
    tstr.push_back(Token{0});

    for (int i = 1; i < 10; ++i) {
        tstr.push_back(Token{predict_next_token(tstr[i - 1])});
    }

    return stringfy(tstr);
}

int main(int argc, char* argv[])
{
    std::ifstream matrix_data{
        "/home/aregmk/Coding/bytepairencoding/trained_data.bin",
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

    token_matrix.resize(vocab_size);
    for (int i = 0; i < vocab_size; ++i) {
        token_matrix[i].resize(vocab_size);
    }

    for (size_t i = 0; i < token_matrix.size(); ++i) {
        for (size_t& count : token_matrix[i]) {
            matrix_data.read(reinterpret_cast<char*>(&count), sizeof(count));
        }
    }

    vocab.resize(vocab_size);
    size_t cur_str_size;
    for (size_t i = 0; i < vocab.size(); ++i) {
        matrix_data.read(reinterpret_cast<char*>(&cur_str_size),
                         sizeof(cur_str_size));

        // read string with size
        vocab[i].resize(cur_str_size);
        matrix_data.read(reinterpret_cast<char*>(vocab[i].data()),
                         cur_str_size);
    }

    size_t index = 0;
    for (;;) {
        std::cout << "Input index:" << std::endl;
        std::cin >> index;
        std::cout << "The token: " << vocab[index] << std::endl;
        for (size_t i = 0; i < vocab_size; ++i) {
            if (token_matrix[index][i] != 0) {
                std::cout << vocab[index] << vocab[i] << std::endl;
            }
        }
    }

// #define DEBUG
#ifdef DEBUG
    std::ofstream diff_data_file{
        "/home/aregmk/Coding/twopairencoding/diff_data.bin", std::ios::binary};
    if (!diff_data_file.is_open()) {
        std::cout << "Opening file failed\n" << std::endl;
        exit(1);
    }

    diff_data_file.write(reinterpret_cast<char*>(&vocab_size),
                         sizeof(vocab_size));

    for (size_t i = 0; i < token_matrix.size(); ++i) {
        for (size_t count : token_matrix[i]) {
            diff_data_file.write(reinterpret_cast<char*>(&count),
                                 sizeof(count));
        }
    }

    cur_str_size = 0;
    for (size_t i = 0; i < vocab.size(); ++i) {
        cur_str_size = vocab[i].size();
        diff_data_file.write(reinterpret_cast<char*>(&cur_str_size),
                             sizeof(cur_str_size));

        diff_data_file.write(reinterpret_cast<char*>(vocab[i].data()),
                             cur_str_size);
    }
#endif

    return 0;
}
