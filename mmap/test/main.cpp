#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    const char* file_path = "test.txt";
    int fd = open(file_path, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    size_t file_size = sb.st_size;

    char* mmapped = (char*)mmap(NULL, file_size,  PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mmapped == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // 走标准输出流程
    printf("mmapped: %.*s\n", (int)file_size, mmapped);

    // 输出内容到指定文件描述符
    ssize_t n = write(STDOUT_FILENO, mmapped, file_size);
    if (n == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    printf("wrote %zd bytes\n", n);
    if (munmap(mmapped, file_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
    close(fd);

    return 0;
}