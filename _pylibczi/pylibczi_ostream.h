//
// Created by Jamie Sherman on 2019-09-19.
//

#ifndef _PYLIBCZI_PYLIBCZI_OSTREAM_H
#define _PYLIBCZI_PYLIBCZI_OSTREAM_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "inc_libCZI.h"

using namespace std;

ostream &operator<<(ostream &out, const libCZI::CDimCoordinate &cdim);
ostream & operator<<(ostream &out, const libCZI::CDimBounds &bounds);

#endif //_PYLIBCZI_PYLIBCZI_OSTREAM_H
