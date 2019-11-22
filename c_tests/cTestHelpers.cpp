//
// Created by Jamie Sherman on 11/20/19.
//

#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#endif


#include "cTestHelpers.h"

int jsOpen(std::string fname_ ){
    int ans = -1;
#ifdef _WIN32
    ans = _open(fname_.c_str(), O_RDONLY);
#else
    ans = open(fname_.c_str(), O_RDONLY);
#endif
    return ans;
}

