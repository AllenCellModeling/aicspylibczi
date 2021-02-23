#ifndef _PYLIBCZI_PYLIBCZI_OSTREAM_H
#define _PYLIBCZI_PYLIBCZI_OSTREAM_H

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "inc_libCZI.h"

using namespace std;

ostream&
operator<<(ostream& out_, const libCZI::CDimCoordinate& plane_coordante_);
ostream&
operator<<(ostream& out_, const libCZI::CDimBounds& plane_coordinate_bounds_);

#endif //_PYLIBCZI_PYLIBCZI_OSTREAM_H
