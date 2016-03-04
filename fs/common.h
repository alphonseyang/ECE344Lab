#ifndef _COMMON_H
#define _COMMON_H

#include <inttypes.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define EXIT(error) do { \
        fprintf(stderr, "%s: %s: %s\n", __FUNCTION__, error, strerror(errno)); \
        fflush(stderr); \
        exit(1); \
 } while (0)

#define WARN(error) do { \
        fprintf(stderr, "%s: %s: %s\n", __FUNCTION__, error, strerror(errno)); \
        fflush(stderr); \
 } while (0)

#define TBD() do {						   \
        printf("%s:%d: %s: please implement this functionality\n", \
               __FILE__, __LINE__, __FUNCTION__);		   \
        return -ENOSYS;						   \
} while (0)

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define DIVROUNDUP(a,b) (((a)+(b)-1)/(b))
#define ROUNDUP(a,b)    (DIVROUNDUP(a,b)*b)

int str_to_offset(const char *str, off_t *offp);
int str_to_size(const char *str, size_t *sizep);

#endif /* _COMMON_H */
