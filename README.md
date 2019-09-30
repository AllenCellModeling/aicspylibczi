# pylibczi
[![C++ Build & Test](https://github.com/AllenCellModeling/pylibczi/workflows/C%2B%2B%20Build%20%26%20Test/badge.svg)](https://github.com/AllenCellModeling/pylibczi/actions)
[![Python Build & Test](https://github.com/AllenCellModeling/pylibczi/workflows/Python%20Build%2C%20Test%2C%20%26%20Lint/badge.svg)](https://github.com/AllenCellModeling/pylibczi/actions)
[![codecov](https://codecov.io/gh/AllenCellModeling/pylibczi/branch/feature/pybind11/graph/badge.svg)](https://codecov.io/gh/AllenCellModeling/pylibczi)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)


Python module to expose [libCZI](https://github.com/zeiss-microscopy/libCZI) functionality for reading (subset of) Zeiss CZI files and meta-data.

## Installation

The preferred installation method is with `pip install`.
This will intall the pylibczi python module and extension binaries ([hosted on PyPI](https://pypi.org/project/pylibczi/)):
```
pip install pylibczi
```

## Usage

For example usage, see the [`Example_Usage.ipynb`](Example_Usage.ipynb).
To try out the notebook you need to launch `jupyter notebook` and then open the `Example_usage.ipynb`
This shows how to work with a standard CZI file and a Mosaic CZI file.

## Documentation

[Documentation](https://pylibczi.readthedocs.io/en/latest/index.html) is available on readthedocs.

## Build

Use these steps to build and install pylibczi locally:

* Clone the repository including submodules (`--recurse-submodules`).
* Requirements:
  * libCZI requires a c++11 compatible compiler. Built & Tested with clang.
  * Development requirements are those required for libCZI: **libpng**, **zlib**
  * Install the package:
    ```
    pip install .
    pip install -e .[dev] # for development (-e means editable so changes take effect when made)
    pip install .[all] # for everything including jupyter notebook to work with the Example_Usage above
    ```
  * libCZI is automatically built as a submodule and linked statically to pylibczi.

## License

The GPLv3 license is a consequence of libCZI which imposes GPLv3. If you wish to use libCZI or this derivative in
a commercial product you'll need to talk to Zeiss.
