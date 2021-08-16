#include <string.h>
#include <string>

#include <libxy/version.h>

LIB_XY_EXPORTS int get_libxy_version(char *buffer, int buffer_size) {
  const auto verLength = strlen(LIBXY_VER);
  if (buffer_size < verLength) {
    return verLength;
  }
  strcpy(buffer, LIBXY_VER);
  return 0;
}