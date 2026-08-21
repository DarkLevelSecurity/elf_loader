#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Simple ELF loader:
 * - reads a local ELF file (`payload.elf`) into heap memory,
 * - creates an anonymous in-memory file descriptor via `memfd_create`,
 * - writes the ELF bytes into that memfd,
 * - executes the memfd by calling `execv("/proc/self/fd/<fd>", NULL)`.
 *
*/

#ifndef SYS_memfd_create
  #define SYS_memfd_create 319
#endif

/* Flag: close the memfd on exec to avoid leaking into child processes. */
#define MFD_CLOEXEC 0x0001U

/*
 * Small wrapper around the raw syscall number for memfd_create. This
 * abstracts the syscall invocation so the rest of the code can call
 * `memfd_create()` like a normal libc function.
 */
int memfd_create(const char *name, unsigned int flags)
{
  return syscall(SYS_memfd_create, name, flags);
}

int get_payload(unsigned char **buffer, size_t *size, const char *file_name);

const char *payload_path = "payload_example.elf";

/*
 * main:
 * - load payload into memory
 * - create memfd and write payload into it
 * - exec the memfd via /proc/self/fd/<fd>
 */
int main()
{
  unsigned char *buffer = NULL;
  size_t size = 0;

  /* Load the payload from disk into `buffer`. Caller owns `buffer`. */
  if (get_payload(&buffer, &size, payload_path) != 0)
  {
    fprintf(stderr, "Failed to load payload\n");
    return 1;
  }

  /* Print diagnostics about loaded payload. */
  printf("buffer size: %zu\n", size);
  printf("buffer address: %p\n", (void *)buffer);

  /* Create an anonymous in-memory file (memfd). */
  int fd = memfd_create("", MFD_CLOEXEC);
  if (fd == -1)
  {
    perror("memfd_create");
    free(buffer);
    return 1;
  }

  printf("memfd_create fd: %d\n", fd);

  /* Write the payload bytes into the memfd. */
  ssize_t written = write(fd, buffer, size);
  if (written != (ssize_t)size)
  {
    perror("write");
    close(fd);
    free(buffer);
    return 1;
  }
  printf("Written %zd bytes to memfd\n", written);

  /* Build the /proc path for the memfd and execute it. */
  char path[64];
  snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
  printf("Executing payload from: %s\n", path);

  /* execv replaces current process; if it returns, an error occurred. */
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
  printf("File size: %zu bytes\n", file_size);

  /* Allocate exactly the file size for binary data. */
  unsigned char *tmp_buffer = malloc(file_size);
  if (!tmp_buffer)
  {
    perror("malloc");
    return -1;
  }

  /* Open the file in binary mode and read into the buffer. */
  FILE *fp = fopen(file_name, "rb");
  if (!fp)
  {
    perror("fopen");
    free(tmp_buffer);
    return -1;
  }

  /* Read the file; for binaries we expect fread to return file_size. */
  size_t n = fread(tmp_buffer, 1, file_size, fp);
  if (n != file_size)
  {
    if (ferror(fp))
      perror("fread");
    fprintf(stderr, "Warning: read %zu of %zu bytes\n", n, file_size);
  }
  fclose(fp);

  /* Transfer ownership of the allocated buffer to the caller. */
  *buffer = tmp_buffer; /* transfer ownership to caller */
  *size = n;
  return 0;
}
