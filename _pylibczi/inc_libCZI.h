//******************************************************************************
//
// libCZI is a reader for the CZI fileformat written in C++
// Copyright (C) 2017  Zeiss Microscopy GmbH
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// To obtain a commercial version please contact Zeiss Microscopy GmbH.
//
//******************************************************************************

#pragma once

#if  defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
// Windows Header Files:
#include <windows.h>
#endif

#include "libCZI/libCZI.h"
#include "libCZI/CZIReader.h"

#include "libCZI/splines.h"

#include "libCZI/bitmapData.h"
#include "libCZI/stdAllocator.h"
#include "libCZI/BitmapOperations.h"

#include "libCZI/CziSubBlockDirectory.h"
#include "libCZI/StreamImpl.h"

#include "libCZI/libCZI_DimCoordinate.h"
#include "libCZI/libCZI_Utilities.h"
