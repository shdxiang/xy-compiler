#include <cstdio>

/* Runtime functions that xy programs can declare with `extern`. They are
   registered with the JIT in main.cpp. */

/* xy functions always return an integer, so printi returns its argument */
extern "C" long long printi(long long val) {
  std::printf("%lld\n", val);
  return val;
}
