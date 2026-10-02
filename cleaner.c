#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define BUFF_SIZE 1 << 20
#define NEW_FILE_NAME_SIZE 1 << 8
#define NEW_FILE_NAME "./clean/cleaned_data_output.txt"

int main(int argc, char* argv[])
{
    char* path = NULL;
    if (argc < 2) {
        fprintf(stderr, "usage: main <path>\n");
        return 1;
    }
    path = argv[1];

    FILE* input = fopen(path, "r");
    if (input == NULL) {
        fprintf(stderr, "fopen: couldn't open file\n");
        return 1;
    }

    FILE* output = fopen(NEW_FILE_NAME, "w+");
    if (output == NULL) {
        fprintf(stderr, "fopen: couldn't create file\n");
        return 1;
    }

    char buffer[BUFF_SIZE] = {0};
    while (fgets(buffer, BUFF_SIZE, input)) {
        // filter
        if (strstr(buffer, "<Media omitted>") != NULL)
            continue;

        // find colon
        char* colon      = NULL;
        char* time_colon = strchr(buffer, ':');
        if (!time_colon)
            continue;

        colon = strchr(time_colon + 1, ':');
        if (!colon)
            continue;

        fprintf(output, "%s", colon + 1);
    }

    return 0;
}
