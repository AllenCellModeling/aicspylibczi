// Make a single platform independent include for unistd.h which is only available on *nix.

// unistd.h does not exist in windows
#if defined(_WIN32) || defined(_WIN64)
#include <stdio.h>
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
const auto& fseeko = _fseeki64;
#else
#include <unistd.h>
#endif
