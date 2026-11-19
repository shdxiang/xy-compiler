#include <cstring>

#include <libxy/version.h>

LIB_XY_EXPORTS int get_libxy_version(char *buffer, int buffer_size) {
  const int required = static_cast<int>(std::strlen(LIBXY_VER)) + 1;
  if (buffer == nullptr || buffer_size < required) {
    return required;
  }
  std::memcpy(buffer, LIBXY_VER, required);
  return 0;
}
