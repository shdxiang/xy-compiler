#ifndef FA70C9C5_E371_4F29_83E8_04D0469AB8EF
#define FA70C9C5_E371_4F29_83E8_04D0469AB8EF

#ifndef MODULE_API
#ifdef MODULE_API_EXPORTS
#if _WIN32
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API __attribute__((dllexport))
#endif
#else
#if _WIN32
#define MODULE_API __declspec(dllimport)
#else
#define MODULE_API __attribute__((dllimport))
#endif
#endif
#else
#define MODULE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

MODULE_API void printi(long long val);

#ifdef __cplusplus
}
#endif

#endif /* FA70C9C5_E371_4F29_83E8_04D0469AB8EF */
