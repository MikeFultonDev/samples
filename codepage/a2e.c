#include "codepage.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  int c;
  while ((c = getchar()) >= 0) {
    putchar(ascii[c]);
  }
  return 0;
}
  
