# pylibczi
[![C++ Build & Test](https://github.com/AllenCellModeling/pylibczi/workflows/C%2B%2B%20Build%20%26%20Test/badge.svg)](https://github.com/AllenCellModeling/pylibczi/actions)
[![Python Build & Test](https://github.com/AllenCellModeling/pylibczi/workflows/Python%20Build%2C%20Test%2C%20%26%20Lint/badge.svg)](https://github.com/AllenCellModeling/pylibczi/actions)
[![codecov](https://codecov.io/gh/AllenCellModeling/pylibczi/branch/feature/pybind11/graph/badge.svg)](https://codecov.io/gh/AllenCellModeling/pylibczi)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)


Python module to expose [libCZI](https://github.com/zeiss-microscopy/libCZI) functionality for reading (subset of) Zeiss CZI files and meta-data.

## Usage

The first example show how to work with a standard CZI file (Single or Multe-Scene). The second example shows how to work with a Mosaic CZI file.

#### Example 1:  Read in a czi and select a portion of the image to display
```python
import pylibczi
import pathlib
from PIL import Image

pth = pathlib.Path('/allen/aics/assay-dev/MicroscopyData/Sue/2019/20190610/20190610_S02-02.czi')
czi = pylibczi.CziFile(pth)

# Get the shape of the data, the coordinate pairs are (start index, size)
dimensions = czi.dims()  # {'Z': (0, 70), 'C': (0, 2), 'T': (0, 146), 'S': (0, 12), 'B': (0, 1)}

# Load the image slice I want from the file
img, shp = czi.read_image(S=4, T=11, C=0, Z=30) 

# shp = [('S', 1), ('T', 1), ('C', 1), ('Z', 1), ('Y', 1300), ('X', 1900)]  # List[(Dimension, size), ...]
# img.shape = (1, 1, 1, 1, 1300, 1900)   # numpy.ndarray

# Normalize the image 
norm_by = np.percentile(img[0, 0, 0,], [50, 99.8])

# Scale the numpy array values and cast them back to integers in the 0 to 255 range
i2 = np.clip((img - norm_by[0])/(norm_by[1]-norm_by[0]), 0, 1)*255

img_disp = Image.fromarray(i2[0,0,0,0,200:1100,500:1000].astype(np.uint8))
```
![Colony Image](colony.png)

#### Example 2:  Read in a mosaic file 
```python
import pylibczi
import pathlib
from PIL import Image

mosaic_file = pathlib.Path('~/Data/20190618_CL001_HB01_Rescan_002.czi').expanduser()
czi = pylibczi.CziFile(mosaic_file)

# Get the shape of the data
dimensions = czi.dims()   # {'C': (0, 5), 'S': (0, 16), 'B': (0, 1)}

czi.is_mosaic()  # True 
 # Mosaic files ignore the S dimension and use an internal mIndex to reconstruct, the scale factor allows one to generate a manageable image
mosaic_data = czi.read_mosaic(C=1, scale_factor=0.1) 

mosaic_data.shape  # (1, 1, 6265, 6998)

norm_by = np.percentile(mosaic_data, [5, 98])
normed_mosaic_data = np.clip((itwo - norm_by[0])/(norm_by[1]-norm_by[0]), 0, 1)*255
img = Image.fromarray(normed_mosaic_data[0,0, 250:750, 250:750].astype(np.uint8))
```
![Mosaic Histology Image](histo.png)

## Installation

The preferred installation method is with `pip install`.
This will install the pylibczi python module and extension binaries ([hosted on PyPI](https://pypi.org/project/pylibczi/)):

`
pip install pylibczi
`

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
  * libCZI is automatically built as a submodule and linked statically into pylibczi.
* Note: If you get the message `EXEC : Fatal Python error : initfsencoding: unable to load the file system codec ... ModuleNotFoundError: No module named 'encodings'` on windows you need to set PYTHONHOME to be the folder the python.exe you are compiling against lives in.


## License

The GPLv3 license is a consequence of libCZI which imposes GPLv3. If you wish to use libCZI or this derivative in
a commercial product you'll need to talk to Zeiss.
