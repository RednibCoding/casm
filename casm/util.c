#define _CRT_SECURE_NO_WARNINGS // needed to suppress warnings about using 'unsafe' functions

#include <string.h>

void strcopy(char* dest, const char* src, size_t count) {
#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
	strncpy_s(dest, count + 1, src, _TRUNCATE);
#else
	strncpy(dest, src, count);
	dest[count] = '\0';
#endif
}