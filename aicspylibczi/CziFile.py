# Parent class for python wrapper to libczi file for accessing Zeiss czi image and metadata.

import io
from pathlib import Path
from typing import BinaryIO, Tuple, Union

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

    def __init__(self, czi_filename: types.FileLike, metafile_out: types.PathLike = '',
                 verbose: bool = False):
        # Convert to BytesIO (bytestream)
        self._bytes = self.convert_to_buffer(czi_filename)
        self.czi_filename = None
        self.metafile_out = metafile_out
        self.czifile_verbose = verbose

        import _aicspylibczi
        self.czilib = _aicspylibczi
        self.reader = self.czilib.Reader(self._bytes)

        self.meta_root = None

    @property
    def shape_is_consistent(self):
        """
        Query if the file shape is consistent across scenes.

        Returns
        -------
        bool
            true if consistent, false the scenes have different dimension shapes

        """
        return self.reader.has_consistent_shape()

    @property
    def dims(self):
        """
        Get the dimensions present the binary data (not the metadata)
        Y and X are included for completeness but can not be used as constraints.

        Returns
        -------
        str
            A string containing Dimensions letters present, ie "BSTZYX"

        """
        return self.reader.read_dims_string()

    def dims_shape(self):
        """
        Get the dimensions for the opened file from the binary data (not the metadata)

        Returns
        -------
        list[dict]
            A list of dictionaries containing Dimension / depth. If the shape is consistent across Scenes then
            the list will have only one Dictionary. If the shape is inconsistent the the list will have a dictionary
             for each Scene. A consistently shaped file with 3 scenes, 7 time-points
            and 4 Z slices containing images of (h,w) = (325, 475) would return
            [
             {'S': (0, 3), 'T': (0,7), 'X': (0, 475), 'Y': (0, 325), 'Z': (0, 4)}
            ].
            The result for a similarly shaped file but with different number of time-points per scene would yield
            [
             {'S': (0, 1), 'T': (0,8), 'X': (0, 475), 'Y': (0, 325), 'Z': (0, 4)},
             {'S': (1, 2), 'T': (0,6), 'X': (0, 475), 'Y': (0, 325), 'Z': (0, 4)},
             {'S': (2, 3), 'T': (0,7), 'X': (0, 475), 'Y': (0, 325), 'Z': (0, 4)}
            ]

        """
        return self.reader.read_dims()

    def scene_bounding_box(self, index: int = -1):
        """
        Get the bounding box of the raw collected data (pyramid 0) from the czifile. if not specified it defaults to
        the first scene

        Parameters
        ----------
        index
             the scene index, omit and it defaults to the first one

        Returns
        -------
        tuple
            (x0, y0, w, h) for the specified scene

        """
        bbox = self.reader.read_scene_wh(index)
        ans = (bbox.x, bbox.y, bbox.w, bbox.h)
        return ans

    def scene_height_by_width(self, index: int = -1):
        """
        Get the size of the scene (Y, X) / (height, width) for the specified Scene. The default is to return
        the size of the first Scene but Zeiss allows scenes to be different sizes thought it is unlikely to encounter.
        This module will warn you on instantiation if the scenes have inconsistent width and height.

        Parameters
        ----------
        index
            specifies the index of the Scene to get the height and width of

        Returns
        -------
        tuple
            (height, width) tuple of the Specified scene.

        """
        box = self.reader.read_scene_wh(index)
        return (box.h, box.w)

    @property
    def size(self):
        """
        This returns the Size of each dimension in the dims string. So if S had valid indexes of [0, 1, 2, 3, 4]
        the returned tuple would have a value of 5 in the same position as the S occurs in the dims string.

        Returns
        -------
        tuple
            a tuple of dimension sizes. If the data has inconsistent shape the list will only contain -1 values and
            the user needs to use dims_shape() to get the indexes.

        """
        return tuple(self.reader.read_dims_sizes())

    def is_mosaic(self):
        """
        Test if the loaded file is a mosaic file

        Returns
        -------
        bool
            True | False ie is this a mosaic file
        """
        return self.reader.is_mosaic()

    @staticmethod
    def convert_to_buffer(file: types.FileLike) -> Union[BinaryIO, np.ndarray]:
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

    @property
    def meta(self):
        """
        Extract all metadata from czifile.

        Returns
        -------
        lxml.etree
            metadata as an xml etree

        """
        if self.meta_root is None:
            meta_str = self.reader.read_meta()
            self.meta_root = etree.fromstring(meta_str)

        if self.metafile_out:
            metastr = etree.tostring(self.meta_root, pretty_print=True).decode('utf-8')
            with open(self.metafile_out, 'w') as file:
                file.write(metastr)
        return self.meta_root

    def read_subblock_metadata(self, **kwargs):
        """
        Read the subblock specific metadata, ie time subblock was acquired / position at acquisition time etc.

        Parameters
        ----------
        kwargs
            The keywords below allow you to specify the dimensions that you wish to match. If you
            under-specify the constraints you can easily end up with a massive image stack.
                       Z = 1   # The Z-dimension.
                       C = 2   # The C-dimension ("channel").
                       T = 3   # The T-dimension ("time").
                       R = 4   # The R-dimension ("rotation").
                       S = 5   # The S-dimension ("scene").
                       I = 6   # The I-dimension ("illumination").
                       H = 7   # The H-dimension ("phase").
                       V = 8   # The V-dimension ("view").
                       M = 10  # The M_index, this is only valid for Mosaic files!

        Returns
        -------
        [str]
            an array of stings containing the specified subblock metadata

        """
        plane_constraints = self.czilib.DimCoord()
        [plane_constraints.set_dim(k, v) for (k, v) in kwargs.items() if k in CziFile.ZISRAW_DIMS]
        m_index = self._get_m_index_from_kwargs(kwargs)

        return self.reader.read_meta_from_subblock(plane_constraints, m_index)

    def read_image(self, **kwargs):
        """
        Read the subblocks in the CZI file and for any subblocks that match all the constraints in kwargs return
        that data. This allows you to select channels/scenes/time-points/Z-slices etc. Note if passed a BGR image
        then the dims of the object will returned by this function and the implicit BGR channel will be expanded
        into 3 channels. This shape differ from the values of dims(), size(), and dims_shape() as these are returning
        the native shape without changing from BGR_3X to Gray_X.

        Parameters
        ----------
        **kwargs
            The keywords below allow you to specify the dimensions that you wish to match. If you
            under-specify the constraints you can easily end up with a massive image stack.
                 Z = 1   # The Z-dimension.
                 C = 2   # The C-dimension ("channel").
                 T = 3   # The T-dimension ("time").
                 R = 4   # The R-dimension ("rotation").
                 S = 5   # The S-dimension ("scene").
                 I = 6   # The I-dimension ("illumination").
                 H = 7   # The H-dimension ("phase").
                 V = 8   # The V-dimension ("view").
                 M = 10  # The M_index, this is only valid for Mosaic files!

        Returns
        -------
        (numpy.ndarray, [Dimension, Size])
            a tuple of (numpy.ndarray, a list of (Dimension, size)) the second element of the tuple is to make
            sure the numpy.ndarray is interpretable. An example of the list is
            [('S', 1), ('T', 1), ('C', 2), ('Z', 25), ('Y', 1024), ('X', 1024)]
            so if you probed the numpy.ndarray with .shape you would get (1, 1, 2, 25, 1024, 1024).

        """
        plane_constraints = self.czilib.DimCoord()
        [plane_constraints.set_dim(k, v) for (k, v) in kwargs.items() if k in CziFile.ZISRAW_DIMS]
        m_index = self._get_m_index_from_kwargs(kwargs)

        image, shape = self.reader.read_selected(plane_constraints, m_index)
        return image, shape

    def read_mosaic_size(self):
        """
        Get the size of the entire mosaic image, if it's not a mosaic image return (0, 0, -1, -1)

        Returns
        -------
        (int, int, int, int)
            (x, y, w, h) the bounding box of the mosaic image

        """
        if not self.reader.is_mosaic():
            ans = self.czilib.IntRect()
            ans.x = ans.y = 0
            ans.w = ans.h = -1
        else:
            ans = self.reader.mosaic_shape()
        return (ans.x, ans.y, ans.w, ans.h)

    def read_mosaic(self, region: Tuple = None, scale_factor: float = 1.0, **kwargs):
        """
        Reads a mosaic file and returns an image corresponding to the specified dimensions. If the file is more than
        a 2D sheet of pixels, meaning only one channel, z-slice, time-index, etc then the kwargs must specify the
        dimension with more than one possible value.

        **Example:** Read in the C=1 channel of a mosaic file at 1/10th the size

            czi = CziFile(filename)
            img = czi.read_mosaic(scale_factor=0.1, C=1)

        Parameters
        ----------
        region
            A rectangle specifying the extraction box (x, y, width, height) specified in pixels
        scale_factor
            The amount to scale the data by, 0.1 would mean an image 1/10 the height and width of native, if you
            get distortions it seems to be due to a bug in Zeiss's libCZI I'm trying to track it down but for now
            if you use scale_factor=1.0 it should work properly.
        kwargs
            The keywords below allow you to specify the dimension plane that constrains the 2D data. If the
            constraints are underspecified the function will fail. ::
                    Z = 1   # The Z-dimension.
                    C = 2   # The C-dimension ("channel").
                    T = 3   # The T-dimension ("time").
                    R = 4   # The R-dimension ("rotation").
                    S = 5   # The S-dimension ("scene").
                    I = 6   # The I-dimension ("illumination").
                    H = 7   # The H-dimension ("phase").
                    V = 8   # The V-dimension ("view").

        Returns
        -------
        numpy.ndarray
            (1, height, width)
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

    def _get_m_index_from_kwargs(self, kwargs):
        m_index = -1
        if 'M' in kwargs:
            if not self.is_mosaic():
                raise self.czilib.PylibCZI_CDimCoordinatesOverspecifiedException(
                    "M Dimension is specified but the file is not a mosaic file!"
                )
            m_index = kwargs.get('M')
        return m_index
