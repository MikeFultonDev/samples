#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

static void syntax(const char* pgm) {
  fprintf(stderr, "%s <read|write> <small|medium|large>\n", pgm);
}

typedef enum {
  READ=1,
  WRITE=2
} IOType;

typedef enum {
  SMALL=1,
  MEDIUM=2,
  LARGE=3
} IOSize;

const char* smallfile="/tmp/small.txt";
const char* mediumfile="/tmp/medium.txt";
const char* largefile="/tmp/large.txt";

static int readfile(IOSize size, const char* file) {
  int bytes=0;
  char* buffer;
  int fd;

  switch (size) {
    case SMALL: bytes=1024; break;
    case MEDIUM: bytes=1024*1024; break;
    case LARGE: bytes=500*1024*1024; break;
    default: break;
  }
  buffer = malloc(bytes);
  if (!buffer) {
    fprintf(stderr, "Unable to allocate buffer for read\n");
    return 8;
  }
  fd = open(file, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s for read\n", file);
    return 4;
  }
  if (read(fd, buffer, bytes) != bytes) {
    fprintf(stderr, "Unable to read %d bytes from %s\n", bytes, file);
  }
  close(fd);

  return 0;
}

static int writefile(IOSize size, const char* file) {
  int bytes=0;
  char* buffer;
  int fd;

  switch (size) {
    case SMALL: bytes=1024; break;
    case MEDIUM: bytes=1024*1024; break;
    case LARGE: bytes=500*1024*1024; break;
    default: break;
  }
  buffer = malloc(bytes); /* on purpose, leave it as garbage characters */
  if (!buffer) {
    fprintf(stderr, "Unable to allocate buffer for write\n");
    return 8;
  }
  fd = open(file, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
  if (fd < 0) {
    fprintf(stderr, "Unable to open %s for write\n", file);
    return 4;
  }
  if (write(fd, buffer, bytes) != bytes) {
    fprintf(stderr, "Unable to read %d bytes from %s\n", bytes, file);
  }
  close(fd);

  return 0;
}

static int readtest(IOSize size) {
  switch(size) {
    case SMALL:
      readfile(size, smallfile);
      break;
    case MEDIUM:
      readfile(size, mediumfile);
      break;
    case LARGE:
      readfile(size, largefile);
      break;
    default:
      break;
  }
  return 0;
}

static int writetest(IOSize size) {
  switch(size) {
    case SMALL:
      writefile(size, smallfile);
      break;
    case MEDIUM:
      writefile(size, mediumfile);
      break;
    case LARGE:
      writefile(size, largefile);
      break;
    default:
      break;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  IOType type;
  IOSize size;
  char* stype;
  char* ssize;

  if (argc != 3) {
    syntax(argv[0]);
    return 4;
  }
  stype = argv[1];
  ssize = argv[2];

  if (!strcmp(stype, "read")) {
    type=READ;
  } else if (!strcmp(stype, "write")) {
    type=WRITE;
  } else {
    syntax(argv[0]);
    return 4;
  }

  if (!strcmp(ssize, "small")) {
    size=SMALL;
  } else if (!strcmp(ssize, "medium")) {
    size=MEDIUM;
  } else if (!strcmp(ssize, "large")) {
    size=LARGE;
  } else {
    syntax(argv[0]);
    return 4;
  }

  switch (type) {
    case READ: readtest(size); break;
    case WRITE: writetest(size); break;
    default: break;
  }

  return 0;
}

