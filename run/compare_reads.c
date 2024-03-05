#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 4096

void read_file(const char *file_path) {
	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	char buffer[BUFFER_SIZE];
	while (read(fd, buffer, BUFFER_SIZE) > 0);

	close(fd);
}

double measure_operation_time(void (*operation)(const char *), const char *file_path) {
	struct timespec start, end;

	clock_gettime(CLOCK_MONOTONIC, &start);
	operation(file_path);
	clock_gettime(CLOCK_MONOTONIC, &end);

	double time_taken = end.tv_sec - start.tv_sec;
	time_taken += (end.tv_nsec - start.tv_nsec) / 1000000000.0;

	return time_taken;
}

int main() {
        double myfs_total = 0.0;
        for(int i = 0; i < 10; ++i) {
                char path[60];
                snprintf(path, sizeof(path), "mountdir/test%c", i + '0');
                myfs_total += measure_operation_time(read_file, path);
        }
        printf("MyFS read operation time %f seconds\n", myfs_total / 10);

	double nfs_total = 0.0;
	for(int i = 0; i < 10; ++i) {
		char path[60];
		snprintf(path, sizeof(path), "/u/mve/test%c", i + '0');
		nfs_total += measure_operation_time(read_file, path);
	}
	printf("NFS read operation time %f seconds\n", nfs_total / 10);

	return 0;
}

