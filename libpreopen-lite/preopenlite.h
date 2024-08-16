#ifndef PREOPENLITE_H
#define PREOPENLITE_H

#include <fcntl.h>

// This is just to ensure the header is C++ compatible
#ifdef __cplusplus
extern "C" {
#endif

int open(const char *pathname, int flags, ...);

#ifdef __cplusplus
}
#endif

#endif // PREOPENLITE_H
