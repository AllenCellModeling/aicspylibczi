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

import time

import numpy as np
from lxml import etree as etree


# import os

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

    # how many calibration markers to read in, this should essentially be a constant
    nmarkers = 3

    # xxx - likely this is a Zeiss bug,
    #   units for the scale in the xml file are not correct (says microns, given in meters)
    scale_units = 1e6

    ### Dims as defined in libCZI
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
    ZISRAW_DIMS = {'Z', 'C', 'T', 'R', 'S', 'I', 'H', 'V'}

    def __init__(self, czi_filename, metafile_out='', use_pylibczi=True, verbose=False):
        self.czi_filename = czi_filename
        self.metafile_out = metafile_out
        self.czifile_verbose = verbose

        # whether to use czifile or pylibczi for reading the czi file.
        self.use_pylibczi = use_pylibczi
        if use_pylibczi:
            import _pylibczi
            self.czilib = _pylibczi
        else:
            # https://www.lfd.uci.edu/~gohlke/code/czifile.py.html
            import czifile
            self.czilib = czifile

    def read_meta(self):
        """Extract all metadata from czifile.

        Attributes Modified:
            meta_root (etree): xml class containing root of the extracted meta data.

        """
        if self.use_pylibczi:
            self.meta_root = etree.fromstring(self.czilib.cziread_meta(self.czi_filename))
        else:
            # get the root of the metadata xml
            #root = ET.fromstring(metastr) # to convert to python etree
            self.meta_root = self.czi.metadata.getroottree()

        if self.metafile_out:
            metastr = etree.tostring(self.meta_root, pretty_print=True).decode('utf-8')
            with open(self.metafile_out, 'w') as file:
                file.write(metastr)

    def read_image(self):
        """Read image data from all subblocks and create single montaged image.

        Returns:
          |  (m,n,nchan ndarray):  Montaged image from all subblocks.

        """

        if self.czifile_verbose:
            print('Loading czi image for all scenes'); t = time.time()

        if self.use_pylibczi:
            # xxx - this does not work for czifiles for which the subblocks have no dimension label.
            #   additionally it seems not possible to create an accessor without specifying a dimension label / index.
            #img = self.czilib.cziread_scene(self.czi_filename, -np.ones((1,), dtype=np.int64))
            imgs, coords = self.czilib.cziread_allsubblocks(self.czi_filename)
            if len(imgs) > 1:
                # xxx - was not clear what to do in the cases of many subblocks of different sizes.
                #   could not find any other subblock attribute to indicate what the difference is between them.
                #   only plotting the images that are the majority size gave the result closest to loading in Zen.
                img, _ = CziFile._montage(imgs, coords, mode_size_only=True)
            else:
                img = imgs[0]
        else:
            # (?, scenes, ?, xdim, ydim, colors?)
            img = np.squeeze(self.czilib.CziFile(self.czi_filename).asarray())
            assert( img.ndim <= 3 ) # xxx - other dims?

        if self.czifile_verbose:
            print('\tdone in %.4f s' % (time.time() - t, ))
            print('\tAll scenes size is %d x %d' % (img.shape[0], img.shape[1]))

        return img

    def read_mosaic_size(self):
        """
        Get the size of the entire mosaic image
        :return: (x, y, w, h)
        """
        shape = self.czilib.cziread_mosaic_shape(str(self.czi_filename))
        return shape

    def read_mosaic(self, region = None, scale_factor: float = 1.0, **kwargs):
        """
        reads a mosaic file and returns an image corresponding to the specified dimensions. If the file is more than
        a 2D sheet of pixels, meaning only one channel, z-slice, time-index, etc then the kwargs must specify the
        dimension with more than one possible value.

        :param region: a rectangle specifying the extraction box (x, y, width, height) specified in pixels
        :param scale_factor: amount to scale the data by, 0.1 would mean an image 1/10 the height and width of native
        :param kwargs: The keywords below allow you to specify the dimension plane that constrains the 2D data. If the
        constraints are underspecified the function will fail.
            Z = 1   # The Z-dimension.
            C = 2   # The C-dimension ("channel").
            T = 3   # The T-dimension ("time").
            R = 4   # The R-dimension ("rotation").
            S = 5   # The S-dimension ("scene").
            I = 6   # The I-dimension ("illumination").
            H = 7   # The H-dimension ("phase").
            V = 8   # The V-dimension ("view").
        """
        plane_constraints = {k: v for (k, v) in kwargs.items() if k in CziFile.ZISRAW_DIMS}

        img = self.czilib.cziread_mosaic(str(self.czi_filename), plane_constraints, region, scale_factor)

        return img

    @staticmethod
    def plot_image(image, figno=1, doplots_ds=1, reduce=np.mean, interp_string='nearest', show=True):
        """Generic image plot using matplotlib.

        Kwargs:
          |  figno (int): Figure number to use.
          |  doplots_ds (int): Downsampling reduce factor before plotting.
          |  reduce (func): Function to use for block-reduce downsampling.
          |  interp_string (str): Interpolation string for matplotlib imshow.
          |  show (bool): Whether to show images or return immediately.

        """
        from matplotlib import pylab as pl
        import skimage.measure as measure

        if image.ndim == 3 and issubclass(image.dtype.type, np.integer):
            #image = image / np.iinfo(image.dtype).max
            image = image / image.max()
            # all the zeiss color formats are bgr, not rgb
            image = image[:,:,[2,1,0]]
            assert(doplots_ds==1) # block reduce on color images not implemented here

        img_ds = measure.block_reduce(image, block_size=tuple([doplots_ds for x in range(image.ndim)]),
                                      func=reduce).astype(image.dtype) if doplots_ds > 1 else image

        pl.figure(figno)
        ax = pl.subplot(1,1,1)
        ax.imshow(img_ds,interpolation=interp_string, cmap='gray')
        pl.axis('off')

        if show: pl.show()

    # https://stackoverflow.com/questions/43554819/find-most-frequent-row-or-mode-of-a-matrix-of-vectors-python-numpy
    @staticmethod
    def _mode_rows(a):
        a = np.ascontiguousarray(a)
        void_dt = np.dtype((np.void, a.dtype.itemsize * np.prod(a.shape[1:])))
        _,ids, count = np.unique(a.view(void_dt).ravel(), \
                                    return_index=1,return_counts=1)
        largest_count_id = ids[count.argmax()]
        most_frequent_row = a[largest_count_id]
        return most_frequent_row

    @staticmethod
    def _montage(images, coords, bg=0, mode_size_only=False):
        nimages = len(images)

        # images should be the same datatype, get the datatype.
        sel = np.array([x is None for x in images])
        fnz = np.nonzero(np.logical_not(sel))[0][0]
        img_dtype = images[fnz].dtype

        # get the sizes of all the images.
        img_shapes = np.vstack(np.array([x.shape if x is not None else (0,0) for x in images]))

        if mode_size_only:
            r = CziFile._mode_rows(img_shapes)
            sel = (r != img_shapes).any(1)
            img_shapes[sel,:] = -1

        # calculate size and allocate output image.
        corners = coords.astype(np.double, copy=True)
        corners -= np.nanmin(corners, axis=0)
        sz_out = np.ceil(np.nanmax(corners + img_shapes[:,::-1], axis=0)).astype(np.int64)[::-1]
        image = np.empty(sz_out, dtype=img_dtype); image.fill(bg)

        # convert the image corners to subscripts.
        corners = np.round(corners).astype(np.int64)
        c = corners[:,::-1]

        for i in range(nimages):
            if images[i] is None or not (img_shapes[i,:] > 0).all(): continue
            s = img_shapes[i,:]
            image[c[i, 0]:c[i, 0] + s[0], c[i, 1]:c[i, 1] + s[1]] = images[i]

        return image, corners
