#include "codepage.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  int c;
  while ((c = getchar()) >= 0) {
    putchar(ebcdic[c]);
  }
  return 0;
}
  
