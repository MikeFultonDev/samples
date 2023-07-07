#include "accessid.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

AccessIDError accessid_check(const char* id, int altterminator, const char** end) 
{
  size_t i=0;
  size_t idlen=0;
  int needcheck=0;
  char in;

  do {
    in=id[i];
    switch (in) {
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
        ++idlen;
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
        ++idlen;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        ++idlen;
        break;
      case '$':
      case '#':
      case '@':
        ++idlen;
        break;
      default:
        needcheck=1;
        break;
    }
    if (idlen > 8) {
      return AccessIDTooLong;
    }
    if (needcheck) {
      needcheck=0;
      if (in == '\0' || in == altterminator) {
        break;
      }
      return AccessIDInvChar;
    }
    ++i; 
  } while (1);

  if (idlen == 0) {
    return AccessIDTooShort;
  }
  if (end) {
    *end = &id[i];
  }
  return AccessIDOK;
}

char* normalize_id(const char* in, char* out)
{
  size_t len = strlen(in);
  size_t i;
  for (i=0; i<=len; ++i) {
    if (islower(in[i])) {
      out[i] = toupper(in[i]);
    } else {
      out[i] = in[i];
    }
  }
  return out;
}

void paccessiderror(AccessIDError err)
{
  switch(err) {
    case AccessIDOK:
      fprintf(stderr, "No error");
      break;
    case AccessIDInvChar:
      fprintf(stderr, "Invalid id: name must only contain with alphanumeric or special characters\n");
      break;
    case AccessIDTooShort:
      fprintf(stderr, "Invalid id: id must be at least 1 character\n");
      break;
    case AccessIDTooLong:
      fprintf(stderr, "Invalid id: id must be no more than 8 characters\n");
      break;
    default:
      fprintf(stderr, "Unexpected id error: %d\n", err);
      break;
  }
}
