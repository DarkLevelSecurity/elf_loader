#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MFD_CLOSEXEC 0x0001U

int memfd_creaet(const char *name, unsigned int flags)
{
    return syscall(SYS_memfd_create, name, flags);
}

int get_payload(unsigned char **buffer, size_t *size, const char *filename);

const char *payload_name = "payload_example.elf";

int main()
{
    unsigned char *buffer = NULL;
    size_t size = 0;
    if (get_payload(&buffer, &size, payload_name) != 0)
    {
        fprintf(stderr, "Failed to get the payload\n");
        return 1;
    }

    printf("Buffer size: %zu\n", size);
    printf("Buffer address: %p\n", (void *)buffer);

    int fd = memfd_creaet("", MFD_CLOSEXEC);
    if (fd == -1)
    {
        perror("memfd_create");
        free(buffer);
        return 1;
    }

    ssize_t written = write(fd, buffer, size);
    if (written != (ssize_t)size)
    {
        perror("write");
        close(fd);
        free(buffer);
        return 1;
    }

    printf("Wrote %zd byte from buffer to memfd\n", written);

    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    printf("Executing payload from %s", path);

    execv(path, NULL);

    perror("execv");
    close(fd);

    free(buffer);
    return 0;
}

int get_payload(unsigned char **buffer, size_t *size, const char *file_name)
{
    struct stat sb;
    if (stat(file_name, &sb) != 0)
    {
        perror("stat");
        return -1;
    }

    size_t file_size = sb.st_size;
    printf("File size is: %zu bytes", file_size);

    unsigned char *tmp_buffer = malloc(file_size);
    if (!tmp_buffer)
    {
        perror("malloc");
        return -1;
    }

    FILE *file = fopen(file_name, "rb");
    if (file == NULL)
    {
        perror("fopen");
        free(tmp_buffer);
        return -1;
    }

    size_t n = fread(tmp_buffer, 1, file_size, file);
    if (n != file_size)
    {
        if (ferror(file))
            perror("fread");
        fprintf(stderr, "Warning: read %zu of %zu bytes\n", n, file_size);
    }
    fclose(file);

    *buffer = tmp_buffer;
    *size = n;
    return 0;
}