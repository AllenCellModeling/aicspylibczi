# This file is part of pylibczi.
# Copyright (c) 2018 Center of Advanced European Studies and Research (caesar)
#
# pylibczi is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# pylibczi is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pylibczi.  If not, see <https://www.gnu.org/licenses/>.

# Parent class for python wrapper to libczi file for accessing Zeiss czi image and metadata.

import io
from pathlib import Path
from typing import Tuple

import numpy as np
from lxml import etree

from . import types


class CziFile(object):
    """Zeiss CZI file object.

    Args:
      |  czi_filename (str): Filename of czifile to access.

    Kwargs:
      |  metafile_out (str): Filename of xml file to optionally export czi meta data to.
      |  use_pylibczi (bool): Set to false to use Christoph Gohlke's czifile reader instead of libCZI.
      |  verbose (bool): Print information and times during czi file access.

    .. note::

       Utilizes compiled wrapper to libCZI for accessing the CZI file.

    """

    # xxx - likely this is a Zeiss bug,
    #   units for the scale in the xml file are not correct (says microns, given in meters)
    # scale_units = 1e6

    # Dims as defined in libCZI
    #
    # Z = 1  # The Z-dimension.
    # C = 2  # The C-dimension ("channel").
    # T = 3  # The T-dimension ("time").
    # R = 4  # The R-dimension ("rotation").
    # S = 5  # The S-dimension ("scene").
    # I = 6  # The I-dimension ("illumination").
    # H = 7  # The H-dimension ("phase").
    # V = 8  # The V-dimension ("view").
    ####
    ZISRAW_DIMS = {'Z', 'C', 'T', 'R', 'S', 'I', 'H', 'V', 'B'}

    def __init__(self, czi_filename: types.FileLike, metafile_out: types.PathLike = '', use_pylibczi: bool = True,
                 verbose: bool = False):
        # Convert to BytesIO (bytestream)
        self._bytes = self.convert_to_buffer(czi_filename)
        self.czi_filename = None
        self.metafile_out = metafile_out
        self.czifile_verbose = verbose

        import _pylibczi
        self.czilib = _pylibczi
        self.reader = self.czilib.Reader(self._bytes)

    def dims(self):
        """
        Get the dimensions for the opened file from the binary data (not the metadata
        :return: A dictionary containing Dimension / depth, a file with 3 scenes, 7 time-points and 4 Z slices would have
        ::
            {'S': 3, 'T': 7, 'Z':4}
        """
        return self.reader.read_dims()

    def is_mosaic(self):
        """
        Test if the loaded file is a mosaic file
        :returns: True | False
        """
        return self.reader.is_mosaic()

    @staticmethod
    def convert_to_buffer(file: types.FileLike) -> io.BufferedIOBase:
        # Check path
        print(file)
        if isinstance(file, (str, Path)):
            # This will both fully expand and enforce that the filepath exists
            f = Path(file).expanduser().resolve(strict=True)

            # This will check if the above enforced filepath is a directory
            if f.is_dir():
                raise IsADirectoryError(f)

            return open(f, "rb")

        # Convert bytes
        elif isinstance(file, bytes):
            return io.BytesIO(file)

        # Set bytes
        elif isinstance(file, io.BytesIO):
            return file

        elif isinstance(file, io.BufferedReader):
            return file

        # Special case for ndarray because already in memory
        elif isinstance(file, np.ndarray):
            return file

        # Raise
        else:
            raise TypeError(
                f"Reader only accepts types: [str, pathlib.Path, bytes, io.BytesIO], received: {type(file)}"
            )

    def read_meta(self):
        """Extract all metadata from czifile.

        :returns: metadata as an xml etree
        """
        meta_str = self.reader.read_meta()
        self.meta_root = etree.fromstring(meta_str)

        if self.metafile_out:
            metastr = etree.tostring(self.meta_root, pretty_print=True).decode('utf-8')
            with open(self.metafile_out, 'w') as file:
                file.write(metastr)
        return self.meta_root

    def read_image(self, m_index: int = -1, **kwargs):
        """
        Read the subblocks in the CZI file and for any subblocks that match all the constraints in kwargs return that data.
        This allows you to select channels/scenes/timepoints/Z-slices etc.

        :param m_index: If it's a mosaic file and you wish to select specific M-indexs then use this otherwise ignore it.
        :param kwargs: The keywords below allow you to specify the dimensions that you wish to match. If you underspecify
            the constraints you can easily end up with a massive image stack. ::
                       Z = 1   # The Z-dimension.
                       C = 2   # The C-dimension ("channel").
                       T = 3   # The T-dimension ("time").
                       R = 4   # The R-dimension ("rotation").
                       S = 5   # The S-dimension ("scene").
                       I = 6   # The I-dimension ("illumination").
                       H = 7   # The H-dimension ("phase").
                       V = 8   # The V-dimension ("view").

        :returns: a tuple of (numpy.ndarray, a list of (Dimension, size)) the second element of the tuple is to make sure the numpy.ndarray
            is interpretable. An example of the list is ::
                # [('S', 1), ('T', 1), ('C', 2), ('Z', 25), ('Y', 1024), ('X', 1024)]
            # so if you probed the numpy.ndarray with .shape you would get (1, 1, 2, 25, 1024, 1024).
        """
        plane_constraints = self.czilib.DimCoord()
        [plane_constraints.set_dim(k, v) for (k, v) in kwargs.items() if k in CziFile.ZISRAW_DIMS]
        image, shape = self.reader.read_selected(plane_constraints, m_index, True)
        return image, shape

    def read_mosaic_size(self):
        """
        Get the size of the entire mosaic image, if it's not a mosaic image return (0, 0, -1, -1)

        :returns: (x, y, w, h)
        """
        if not self.reader.is_mosaic():
            ans = self.czilib.IntRect()
            ans.x = ans.y = 0
            ans.w = ans.h = -1
            return ans

        return self.reader.mosaic_shape()

    def read_mosaic(self, region: Tuple = None, scale_factor: float = 0.1, **kwargs):
        """
        reads a mosaic file and returns an image corresponding to the specified dimensions. If the file is more than
        a 2D sheet of pixels, meaning only one channel, z-slice, time-index, etc then the kwargs must specify the
        dimension with more than one possible value.

        **Example:** Read in the C=1 channel of a mosaic file at 1/10th the size
        ::
            czi = CziFile(filename)
            img = czi.read_mosaic(scale_factor=0.1, C=1)

        :param region: a rectangle specifying the extraction box (x, y, width, height) specified in pixels
        :param scale_factor: amount to scale the data by, 0.1 would mean an image 1/10 the height and width of native
        :param kwargs: The keywords below allow you to specify the dimension plane that constrains the 2D data. If the
            constraints are underspecified the function will fail. ::
                    Z = 1   # The Z-dimension.
                    C = 2   # The C-dimension ("channel").
                    T = 3   # The T-dimension ("time").
                    R = 4   # The R-dimension ("rotation").
                    S = 5   # The S-dimension ("scene").
                    I = 6   # The I-dimension ("illumination").
                    H = 7   # The H-dimension ("phase").
                    V = 8   # The V-dimension ("view").
        :returns: numpy.ndarray (1, height, width)
        """
        plane_constraints = self.czilib.DimCoord()
        [plane_constraints.set_dim(k, v) for (k, v) in kwargs.items() if k in CziFile.ZISRAW_DIMS]

        if region is None:
            region = self.czilib.IntRect()
            region.w = -1
            region.h = -1
        else:
            assert (len(region) == 4)
            tmp = self.czilib.IntRect()
            tmp.x = region[0]
            tmp.y = region[1]
            tmp.w = region[2]
            tmp.h = region[3]
            region = tmp
        img = self.reader.read_mosaic(plane_constraints, scale_factor, region)

        return img
