#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

void write_file(const char *file_path, const char *data, size_t data_size)
{
    int fd = open(file_path, O_WRONLY | O_TRUNC);
    if (fd == -1)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (write(fd, data, data_size) != data_size)
    {
        perror("Error writing to file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

double measure_operation_time(void (*operation)(const char *, const char *, size_t), const char *file_path, const char *data, size_t data_size)
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    operation(file_path, data, data_size);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_taken = end.tv_sec - start.tv_sec;
    time_taken += (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    return time_taken;
}

void perform_test(const char *fs_name, size_t file_size, int num_tests)
{
    char *data = malloc(file_size);
    if (!data)
    {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    memset(data, 'M', file_size);

    double total_time = 0.0;
    for (int i = 0; i < num_tests; ++i)
    {
        char path[60];
        snprintf(path, sizeof(path), "%s/test_file_%d_%zuB", fs_name, i, file_size);
        total_time += measure_operation_time(write_file, path, data, file_size);
    }

    printf("%s write operation time for %zuB files: %f seconds\n", fs_name, file_size, total_time / num_tests);
    free(data);
}

int main()
{
    const int num_tests = 10;
    const size_t file_sizes[] = {1024, 1024 * 1024, 10 * 1024 * 1024};

    for (size_t i = 0; i < sizeof(file_sizes) / sizeof(file_sizes[0]); ++i)
    {
        perform_test("mountdir", file_sizes[i], num_tests);
        perform_test("/u/mve", file_sizes[i], num_tests);
    }

    return 0;
}
