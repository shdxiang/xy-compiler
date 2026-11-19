#include <cstdio>

#include <libxy/system/console/printi.h>

/* xy functions always return an integer, so printi returns its argument */
LIB_XY_EXPORTS long long printi(long long val) {
  printf("%lld\n", val);
  return val;
}
