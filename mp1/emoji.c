#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Return your favorite emoji.  Do not allocate new memory.
// (This should **really** be your favorite emoji, we plan to use this later in the semester. :))
char *emoji_favorite() {
  return "🤯";
}


// Count the number of emoji in the UTF-8 string `utf8str`, returning the count.  You should
// consider everything in the ranges starting from (and including) U+1F000 up to (and including) U+1FAFF.
int emoji_count(const unsigned char *utf8str) {
  int count = 0;
  if (utf8str) {
    for (unsigned int i = 0; i < strlen(utf8str); i++) {
      if (utf8str[i] == 0xF0 && utf8str[i + 1] == 0x9F) {
        if (utf8str[i + 2] >= 0x80 && utf8str[i + 2] <= 0xAB && utf8str[i + 3] >= 0x80 && utf8str[i + 3] <= 0xBF) {
          count++;
        }
        i += 3;
      }
    }
  }
  return count;
}


// Return a random emoji stored in new heap memory you have allocated.  Make sure what
// you return is a valid C-string that contains only one random emoji.
char *emoji_random_alloc() {
  char* wmq = (char *)malloc(5 * sizeof(char));
  wmq[0] = 0xF0;
  wmq[1] = 0x9F;
  wmq[2] = (rand() & 0x2B) | 0x80;
  wmq[3] = (rand() & 0x3F) | 0x80;
  wmq[4] = '\0';
  return wmq;
}


// Modify the UTF-8 string `utf8str` to invert the FIRST character (which may be up to 4 bytes)
// in the string if it the first character is an emoji.  At a minimum:
// - Invert "😊" U+1F60A ("\xF0\x9F\x98\x8A") into ANY non-smiling face.
// - Choose at least five more emoji to invert.
void emoji_invertChar(unsigned char *utf8str) {
  for (unsigned int i = 0; i < strlen(utf8str); i++) {
    if (utf8str[i] == 0xF0 && utf8str[i+1] == 0x9F) {
      if (utf8str[i+2] == 0x98 && utf8str[i+3] == 0x8A) {
        utf8str[i+3] = 0x93;
      }
      else if (utf8str[i+2] == 0x98 && utf8str[i+3] == 0x81) {
        utf8str[i+3] = 0x94;
      }
      else if (utf8str[i+2] == 0x98 && utf8str[i+3] == 0x8D) {
        utf8str[i+3] = 0x92;
      }
      else if (utf8str[i+2] == 0x98 && utf8str[i+3] == 0x8C) {
        utf8str[i+3] = 0x96;
      }
      else if (utf8str[i+2] == 0x98 && utf8str[i+3] == 0x8B) {
        utf8str[i+3] = 0x9E;
      }
      else if (utf8str[i+2] == 0x98 && utf8str[i+3] == 0x9D) {
        utf8str[i+3] = 0xA3;
      }
      break;
    }
  }
}


// Modify the UTF-8 string `utf8str` to invert ALL of the character by calling your
// `emoji_invertChar` function on each character.
void emoji_invertAll(unsigned char *utf8str) {
  if (utf8str) {
    for (unsigned int i = 0; i < strlen(utf8str); i++) {
      emoji_invertChar(&utf8str[i]);
    }
  }
}


// Reads the full contents of the file `fileName, inverts all emojis, and
// returns a newly allocated string with the inverted file's content.
unsigned char *emoji_invertFile_alloc(const char *fileName) {
  FILE *file = fopen(fileName, "r");
  if (file == NULL) {
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  unsigned char *fileContent = (unsigned char *) malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';
  fclose(file);
  emoji_invertAll(fileContent);
  return fileContent;
}
