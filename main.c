#include <bits/posix2_lim.h>
#include <fcntl.h>
#define NOB_IMPLEMENTATION
#include "vendor/nob.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define ttp(x) 1 << x

#define VOCAB_SIZE 10000
#define BUFF_SIZE ttp(12)

static char* vocabulary[VOCAB_SIZE] = {0};
static size_t vocab_end             = 0;

typedef struct {
    size_t* items;
    size_t capacity;
    size_t count;
} TokenArr;

int64_t exists(const char* token)
{
    for (int i = 0; i < vocab_end; ++i) {
        if (strcmp(token, vocabulary[i]) == 0) {
            return i;
        }
    }

    return -1;
}

TokenArr init_vocab(const char* str)
{
    TokenArr tarr = {0};

    for (int i = 0; i < strlen(str); ++i) {
        char cur_token[2] = {0};
        cur_token[0]      = *(str + i);

        int64_t index = exists(cur_token);
        if (index != -1) {
            da_append(&tarr, index);

            continue;
        }

        char* new_token = malloc(sizeof(char) * 2);
        memcpy(new_token, cur_token, 2);

        vocabulary[vocab_end] = new_token;
        da_append(&tarr, vocab_end);
        vocab_end += 1;
    }

    return tarr;
}

void tokenize(TokenArr* tarr)
{
    while (vocab_end < VOCAB_SIZE) {
        size_t max_count   = 0;
        size_t max_pair[2] = {0};
        for (size_t cur_token = 0; cur_token < tarr->count - 2; ++cur_token) {
            size_t count       = 0;
            size_t cur_pair[2] = {tarr->items[cur_token],
                                  tarr->items[cur_token + 1]};
            for (size_t check = cur_token + 1; check < tarr->count - 1;
                 ++check) {
                size_t check_pair[2] = {tarr->items[check],
                                        tarr->items[check + 1]};
                if (cur_pair[0] == check_pair[0] &&
                    cur_pair[1] == check_pair[1]) {
                    count += 1;
                }
            }

            if (count > max_count) {
                max_count   = count;
                max_pair[0] = cur_pair[0], max_pair[1] = cur_pair[1];
            }
        }

        if (max_count == 0) {
            break;
        }

        const char* first_str     = vocabulary[max_pair[0]];
        size_t first_str_size     = strlen(first_str);
        const char* second_str    = vocabulary[max_pair[1]];
        size_t second_str_size    = strlen(second_str);
        size_t max_token_str_size = first_str_size + second_str_size + 1;

        char* max_token_str = malloc(sizeof(char) * max_token_str_size);
        memcpy(max_token_str, first_str, first_str_size);
        memcpy(max_token_str + first_str_size, second_str, second_str_size);
        max_token_str[max_token_str_size - 1] = 0;

        int64_t index = exists(max_token_str);
        if (index != -1) { // exists
            free(max_token_str);
        } else { // doens't exist
            vocabulary[vocab_end] = max_token_str;
            index                 = vocab_end;
            vocab_end += 1;
        }

        TokenArr new_tarr = {0};

        for (int i = 0; i < tarr->count;) {
            size_t cur_pair[2] = {tarr->items[i], tarr->items[i + 1]};
            if (cur_pair[0] == max_pair[0] && cur_pair[1] == max_pair[1]) {
                da_append(&new_tarr, index);
                i += 2;
                continue;
            }

            da_append(&new_tarr, tarr->items[i]);
            i += 1;
        }

        da_free(*tarr);
        *tarr = new_tarr;
    }
}

void free_vocab()
{
    for (int i = 0; i < vocab_end; ++i) {
        free(vocabulary[i]);
    }
}

int main(int argc, char* argv[])
{
    char* path = NULL;
    if (argc < 2) {
        fprintf(stderr, "usage: main <path>\n");
        return 1;
    }
    path = argv[1];

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "stat: couldn't open file\n");
        return 1;
    }

    size_t file_size = st.st_size;
    printf("file size: %zu\n", file_size);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open: couldn't open file\n");
        return 1;
    }

    char* data = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);

    TokenArr tarr = init_vocab(data);
    tokenize(&tarr);

    printf("Tokenized String:\n");
    da_foreach(size_t, token, &tarr)
    {
        printf("%zu ", *token);
    }
    printf("\n");

    printf("Token Table:\n");
    for (size_t i = 0; i < vocab_end; ++i) {
        printf("%zu: %s\n", i, vocabulary[i]);
    }

    munmap(data, file_size);
    free_vocab();

    da_free(tarr);

    return 0;
}
