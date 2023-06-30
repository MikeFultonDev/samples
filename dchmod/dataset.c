#include "dataset.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

DatasetError check_dataset(const char* dataset) 
{
  size_t i=0;
  size_t qual_len=0;
  size_t qualifiers=1;
  char in;

  do {
    in=dataset[i];
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
        ++qual_len;
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
        ++qual_len;
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
      case '-':
        if (qual_len == 0) {
          return DatasetInvCharStart;
        }
        ++qual_len;
        break;
      case '$':
      case '#':
      case '@':
        ++qual_len;
        break;
      case '.':
        if (qual_len == 0) {
          if (i == 0) {
            return DatasetInvDotStart;
          } else {
            return DatasetInvSuccessive;
          } 
        }
        qual_len=0;
        ++qualifiers;
        break;
      case '\0':
        if (qual_len == 0) {
          return DatasetInvDotEnd;
        }
        if (qualifiers < 2) {
          return DatasetInvMinQual;
        }
        if (i > 44) {
          return DatasetNameTooLong;
        }
        qual_len=0;
        ++qualifiers;
        break;
    }
    if (qual_len > 8) {
      return DatasetQualifierTooLong;
    }
      
  } while (dataset[i++] != '\0');

  return DatasetOK;
}

char* normalize_dataset(const char* in, char* out)
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

void pdataseterror(DatasetError err)
{
  switch(err) {
    case DatasetOK:
      fprintf(stderr, "No error");
      break;
    case DatasetInvCharStart:
      fprintf(stderr, "Invalid dataset name: qualifier must start with alphabetic or special characters\n");
      break;
    case DatasetInvDotStart:
      fprintf(stderr, "Invalid dataset name: name must not start with .\n");
      break;
    case DatasetInvSuccessive:
      fprintf(stderr, "Invalid dataset name: name must not have successive . characters\n");
      break;
    case DatasetInvDotEnd:
      fprintf(stderr, "Invalid dataset name: name must not end with .\n");
      break;
    case DatasetInvMinQual:
      fprintf(stderr, "Invalid dataset name: dataset must be composed of at least two qualifiers\n");
      break;
    case DatasetNameTooLong:
      fprintf(stderr, "Invalid dataset name: dataset must be no more than 44 characters\n");
      break;
    case DatasetQualifierTooLong:
      fprintf(stderr, "Invalid dataset name: qualifier must be no more than 8 characters\n");
      break;
    default:
      fprintf(stderr, "Unexpected dataset error: %d\n", err);
      break;
  }
}
