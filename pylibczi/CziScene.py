#!/usr/bin/env python

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

# Reader for Zeiss czi file images and associated metadata for parsing "scenes" or "ribbons".

import numpy as np
import time

import scipy.spatial.distance as scidist

from .exceptions import CziFileNotMultiSceneError
from .CziFile import CziFile


class CziScene(CziFile):
    """Zeiss CZI scene image and metadata.

    Access functions allow for reading of czi metadata to extract information for a particular scene.

    Args:
      |  czi_filename (str): Filename of czifile to access scene information in.

    Kwargs:
      |  scene (int): The scene to load in czifile (starting at 1).
      |  ribbon (int): The ribbon to crop to (starting at 1). Negative value disables the ribbon cropping.
      |  metafile_out (str): Filename of xml file to export czi meta data to.
      |  tifffile_out (str): Filename of tiff file to export czi scene image to.
      |  verbose (bool): Print information and times during czi file access.

    .. note::

       Utilizes compiled wrapper to libCZI for accessing the CZI file.

    """

    # xml paths that define the sections and rois
    xml_paths = {
        'ScaleX':"/ImageDocument/Metadata/Scaling/Items/Distance[@Id = 'X']/Value",
        'ScaleY':"/ImageDocument/Metadata/Scaling/Items/Distance[@Id = 'Y']/Value",
        'Calibration':\
            '/ImageDocument/Metadata/Experiment/ExperimentBlocks/AcquisitionBlock/SubDimensionSetups/' +
                'CorrelativeSetup/HolderDocument/Calibration',
        'Scenes':'/ImageDocument/Metadata/Information/Image/Dimensions/S/Scenes',
        'SelectionBox':"/ImageDocument/Metadata/MetadataNodes/MetadataNode/Layers/Layer[@Name = \"Cat_Ribbon\"]"+\
            '/Elements/Rectangle/Geometry',
        'SectionPoints':\
            "/ImageDocument/Metadata/MetadataNodes/MetadataNode/Layers/Layer[@Name = \"CAT_Section\"]/Elements/Polygon",
        'ROIPoints':\
            "/ImageDocument/Metadata/MetadataNodes/MetadataNode/Layers/Layer[@Name = \"CAT_ROI\"]/Elements/Polygon",
        }

    def __init__(self, czi_filename, scene=1, ribbon=0, metafile_out='', tifffile_out='', verbose=False):
        CziFile.__init__(self, czi_filename, metafile_out=metafile_out)
        self.scene, self.ribbon = scene-1, ribbon-1
        self.tifffile_out = tifffile_out
        self.cziscene_verbose = verbose
        self.meta_loaded = False
        self.scene_loaded = False
        self.n_scenes = None
        self.scale = None

        if not self.use_pylibczi:
            # czi file object with image data and meta
            self.czi = self.czilib.CziFile(self.czi_filename)

    def read_scene_meta(self):
        """Extract metadata from czifile relevant for scene.

        .. note::

           Sets class member variables from metadata pertaining to loading the initialized scene (or ribbon).

        """
        load_scene = self.scene

        ### read and parse xml data from czi file
        self.read_meta()

        # how to get paths by searching for tags, for reference:
        #a = self.meta_root.findall('.//Polygon'); print('\n'.join([str(self.meta_root.getpath(x)) for x in a]))
        #a = self.meta_root.findall('.//Rectangle'); print('\n'.join([str(self.meta_root.getpath(x)) for x in a]))

        # get the pixel size
        self.scale = np.zeros((2,), dtype=np.double)
        self.scale[0] = float(self.meta_root.xpath(self.xml_paths['ScaleX'])[0].text)*self.scale_units
        self.scale[1] = float(self.meta_root.xpath(self.xml_paths['ScaleY'])[0].text)*self.scale_units

        # get the bounding box on the scene
        # Could not find bounding box around all the scenes in the xml, which is the bounding box for the images.
        #   The bounding box for the images is not the same as the rectangle defined by the markers.
        #   So, load all the scene position and size information for calculating the bounding box.
        # Empirically determined that this bounding box is only used if there is more than one scene.
        #   xxx - is  this parameterized somewhere in the meta, or just an annoying Zeiss design decision?

        scenes = self.meta_root.xpath(self.xml_paths['Scenes'])
        self.n_scenes = 0 if len(scenes) == 0 else len(scenes[0].findall('Scene'))
        if self.n_scenes == 0:
            raise CziFileNotMultiSceneError("Error, Single Scene File. Use CziFile not CziScene to access the data.")
        scenes = scenes[0].findall('Scene')
        center_positions = np.zeros((self.n_scenes, 2), dtype=np.double)
        contour_sizes = np.zeros((self.n_scenes, 2), dtype=np.double)
        found = False
        for scene in scenes:
            i = int(scene.attrib['Index'])
            center_positions[i, :] = np.array([float(x) for x in scene.find('CenterPosition').text.split(',')])
            contour_sizes[i, :] = np.array([float(x) for x in scene.find('ContourSize').text.split(',')])
            found = (found or (i == load_scene))
        assert(found) # bad scene number
        center_position = center_positions[load_scene,:]
        contour_size = contour_sizes[load_scene,:]
        all_scenes_position = (center_positions - contour_sizes/2).min(axis=0)
        all_scenes_size = (center_positions + contour_sizes/2).max(axis=0)

        # get the marker positions
        marker_points = np.zeros((self.nmarkers,2),dtype=np.double) # xxx - do we need the z-position of the marker?
        markers = self.meta_root.xpath(self.xml_paths['Calibration'])[0]
        for i in range(self.nmarkers):
            marker = markers.findall('.//Marker%d' % (i+1,))
            marker_points[i,0] = float(marker[0].findall('.//X')[0].text)
            marker_points[i,1] = float(marker[0].findall('.//Y')[0].text)

        # get the bounding box on the slice and ROI polygons
        boxes = self.meta_root.xpath(self.xml_paths['SelectionBox'])
        nboxes = len(boxes)
        box_corners = np.zeros((nboxes,2),dtype=np.double)
        box_sizes = np.zeros((nboxes,2),dtype=np.double)
        for box,i in zip(boxes,range(nboxes)):
            box_corners[i,0] = float(box.findall('.//Left')[0].text); box_corners[i,1] = \
                float(box.findall('.//Top')[0].text)
            box_sizes[i,0] = float(box.findall('.//Width')[0].text); box_sizes[i,1] = \
                float(box.findall('.//Height')[0].text)

        # get the section polygons
        polygons = self.meta_root.xpath(self.xml_paths['SectionPoints'])
        npolygons = len(polygons); polygons_points = [None]*npolygons
        polygons_rotation = np.zeros((npolygons,),dtype=np.double)
        #polygons_text_pos = np.zeros((npolygons,2),dtype=np.double)
        for polygon,i in zip(polygons,range(npolygons)):
            polygons_points[i] = np.array([[float(y) for y in x.split(',')] \
                           for x in polygon.findall('.//Points')[0].text.split(' ')])
            polygons_rotation[i] = float(polygon.findall('.//Rotation')[0].text)/180*np.pi # convert to radians
            #polygons_text_pos[i,:] = np.array([float(y) for y in polygon.findall('.//Position')[0].text.split(',')])

        # get the ROI polygons
        polygons = self.meta_root.xpath(self.xml_paths['ROIPoints'])
        nROIs = len(polygons); rois_points = [None]*nROIs
        rois_rotation = np.zeros((nROIs,),dtype=np.double)
        #rois_text_pos = np.zeros((nROIs,2),dtype=np.double)
        for polygon,i in zip(polygons,range(nROIs)):
            rois_points[i] = np.array([[float(y) for y in x.split(',')] \
                           for x in polygon.findall('.//Points')[0].text.split(' ')])
            rois_rotation[i] = float(polygon.findall('.//Rotation')[0].text)/180*np.pi # convert to radians
            #rois_text_pos[i,:] = np.array([float(y) for y in polygon.findall('.//Position')[0].text.split(',')])

        ### calculate coordinate transformations and transform points to image coordinates

        # calculate the rotation angle of the rectangle defined by the markers relative to the global coordinate frame
        # get the two markers that are furthest away from each other
        assert(self.nmarkers==3) # wrote this code assuming the markers are three corners of a rectangle
        D = scidist.squareform(scidist.pdist(marker_points)); #diag_dist = D.max()
        other_inds = np.array(np.unravel_index(np.argmax(D), (self.nmarkers,self.nmarkers)))
        corner_ind = np.setdiff1d(np.arange(3), other_inds)[0]
        # get the rotation angle correct by measuring the angle to the point with the larger x-deviation,
        #   centered on the corner.
        a = marker_points[other_inds[0],:]-marker_points[corner_ind,:]
        b = marker_points[other_inds[1],:]-marker_points[corner_ind,:]
        marker_vector = a if np.abs(a[0]) > np.abs(b[0]) else b
        marker_angle = np.arctan(marker_vector[1]/marker_vector[0])
        c, s = np.cos(marker_angle), np.sin(marker_angle); marker_rotation = np.array([[c, -s], [s, c]])

        # get the coordinates of the other corner of the marker-defined rectangle
        pts = np.dot(marker_rotation.T, marker_points[other_inds,:] - marker_points[corner_ind,:])
        apts = np.abs(pts); inds = np.argmax(apts,axis=0); #marker_rectangle_size = apts.max(axis=0)
        pt = np.zeros((2,),dtype=np.double); pt[0] = pts[inds[0],0]; pt[1] = pts[inds[1],1]
        all_marker_points = np.zeros((self.nmarkers+1,2),dtype=np.double)
        all_marker_points[:3,:] = marker_points
        all_marker_points[3,:] = np.dot(marker_rotation, pt) + marker_points[corner_ind,:]

        # for the marker offset from the global coordinate frome, use the corner closest to the origin
        marker_offset = all_marker_points[np.argmin(np.sqrt((all_marker_points**2).sum(1))),:]

        # convert to pixel coordinates using marker offsets and pixel scale
        # global coordinates to the corner of the bounding box around all the scenes in pixels
        self.all_scenes_corner_pix = ((np.dot(marker_rotation.T, all_scenes_position - marker_offset) + \
                                         marker_offset)/self.scale).astype(np.int64)
        # the size of the bounding box around all the scenes is rotation invariant
        self.all_scenes_size_pix = (all_scenes_size/self.scale).astype(np.int64)
        # coordinates to the corner of the scene bounding box relative to the bounding box around all the scenes
        self.scene_corner_pix = ((np.dot(marker_rotation.T, center_position - contour_size/2 - marker_offset) + \
                             marker_offset)/self.scale).astype(np.int64) - self.all_scenes_corner_pix
        # the size of the scene is rotation invariant
        self.scene_size_pix = (contour_size/self.scale).astype(np.int64)

        # the "selection boxes" are called ribbons in Zeiss terminology, xxx - change variable names
        # selection box is defined relative bounding box around all the scenes but is specified in pixel space
        box_corners_pix = box_corners - self.scene_corner_pix
        box_sizes_pix = box_sizes
        # get the selection boxes that are within the scene
        sel = np.logical_and(box_corners_pix >= 0, box_corners_pix + box_sizes_pix <= self.scene_size_pix).all(axis=1)
        self.box_corners_pix = box_corners_pix[sel,:]; self.box_sizes_pix = box_sizes_pix[sel,:]
        self.nboxes = sel.sum()

        # optionally crop out one of the selection boxes (ribbons)
        if self.ribbon >= 0:
            # this requires a special feature because in rare cases the ribbon was not placed properly.
            # assign each polygon to a ribbon based on proximity and
            #   then get bouding boxes of polygons assigned to the specified ribbon.
            bctrs = box_corners_pix + box_sizes_pix/2
            pmin, pmax = self._polys_to_ribbon_box(polygons_points, bctrs)
            rmin, rmax = self._polys_to_ribbon_box(rois_points, bctrs)

            # take the bounding box that encompasses the ribbon and the calculated polygon bounding boxes
            amin = np.vstack((pmin[None,:]-1, rmin[None,:]-1, box_corners_pix[self.ribbon,:][None,:])).min(0)
            amax = np.vstack((pmax[None,:]+1, rmax[None,:]+1,
                              (box_corners_pix[self.ribbon,:] + box_sizes_pix[self.ribbon,:])[None,:])).max(0)

            self.scene_corner_pix += np.round(amin).astype(np.int64)
            self.scene_size_pix = np.round(amax-amin).astype(np.int64)
            # now there is only one box for the scene
            self.nboxes = 1; self.box_corners_pix = np.zeros((1,2)); self.box_sizes_pix = self.scene_size_pix[None,:]

        # get the polygon and roi points relative to the specified scene or ribbon
        self.polygons_points, self.polygons_rotation = self._transform_polygons(polygons_points, polygons_rotation)
        self.npolygons = len(self.polygons_points)
        self.rois_points, self.rois_rotation = self._transform_polygons(rois_points, rois_rotation)
        self.nROIs = len(self.rois_points)

        self.meta_loaded = True
        if self.cziscene_verbose:
            if self.ribbon >= 0:
                print( '%d polygons and %d ROIs are within scene %d, ribbon %d' % (self.npolygons, self.nROIs,
                                                                                   load_scene+1, self.ribbon+1))
            else:
                print( '%d polygons, %d ROIs and %d ribbons are within scene %d' % (self.npolygons, self.nROIs,
                                                                                    self.nboxes, load_scene+1))

    # helper function for read_scene_meta
    def _polys_to_ribbon_box(self, polygons_points, box_centers):
        npolygons = len(polygons_points)
        # calculate the distance from all polygon centers to all ribbon centers.
        pctrs = np.zeros((npolygons,2), dtype=np.double)
        for i in range(npolygons):
            p = polygons_points[i] - self.scene_corner_pix; m = p.min(0); pctrs[i,:] = m + (p.max(0) - m)/2
        # categorize each polygon as belonging to the closest ribbon center.
        d = scidist.cdist(box_centers,pctrs); pbox = np.argmin(d, axis=0)
        # select all the polygons for the specified ribbon and get bounding box
        inds = np.nonzero(pbox == self.ribbon)[0]
        pmin = np.empty((2,), dtype=np.double); pmin.fill(np.inf)
        pmax = np.empty((2,), dtype=np.double); pmax.fill(-np.inf)
        for i in inds:
            p = polygons_points[i] - self.scene_corner_pix
            m = p.min(0); sel = (m < pmin); pmin[sel] = m[sel]
            m = p.max(0); sel = (m > pmax); pmax[sel] = m[sel]
        return pmin, pmax

    # helper function for read_scene_meta
    def _transform_polygons(self, polygons_points, polygons_rotation):
        npolygons = len(polygons_points)
        # points are are also relative to the scene bounding box, also get center of bounding box arond points.
        # polygons are rotated around the center of the bounding box of the polygon points.
        polygons_inscene = np.ones((npolygons,), dtype=np.bool)
        rpolygons_points = [None]*npolygons
        for i in range(npolygons):
            # correct for scene bounding box so points are relative to the scene itself
            rpolygons_points[i] = polygons_points[i] - self.scene_corner_pix

            # rotation matrices
            c, s = np.cos(polygons_rotation[i]), np.sin(polygons_rotation[i]); R = np.array([[c, -s], [s, c]])

            # rotation centers calculated using the bounding boxes
            m = rpolygons_points[i].min(0); ctr = m + (rpolygons_points[i].max(0) - m)/2

            # center, rotate, then move back to center
            rpolygons_points[i] = np.dot(R, (rpolygons_points[i] - ctr).T).T + ctr

            # determine if the polygon is within the load scene
            polygons_inscene[i] = np.logical_and(rpolygons_points[i] > 0,
                                                 rpolygons_points[i] <= self.scene_size_pix).all()

        # remove polyons outside of scene
        rpolygons_points = [rpolygons_points[x] for x in np.nonzero(polygons_inscene)[0]]
        return rpolygons_points, polygons_rotation[polygons_inscene]

    def read_scene_image(self):
        """Load scene image from czifile. Loads metadata if not currently loaded.

        Args:

        Kwargs:

        Attributes Modified:
          |  img (m,n,nchan ndarray): The loaded image data with data type matching that of the image.

        """
        if not self.meta_loaded: self.read_scene_meta()
        load_scene = self.scene

        ### load the image data and crop to specified scene

        if self.cziscene_verbose:
            print('Loading czi image for scene %d' % (load_scene+1,)); t = time.time()
        docrop = True
        if self.use_pylibczi:
            if self.n_scenes==1:
                # meh, thanks Zeiss, determined empirically, need flag?
                img = self.czilib.cziread_scene(self.czi_filename, np.zeros((1,), dtype=np.int64))
            else:
                docrop = False
                self.img = self.czilib.cziread_scene(self.czi_filename,
                    np.concatenate((self.scene_corner_pix, self.scene_size_pix)))
        else:
            # xxx - is there a way to just read one "scene" without importing all the data?
            #   this question only pertains to CziFile, cziread_scene above does this.
            # (?, scenes, ?, xdim, ydim, colors?)
            img = np.squeeze(self.czi.asarray()[:,load_scene,:,:,:])
            assert( img.ndim == 2 ) # multiple colors or other dims?
        if docrop:
            # crop out the scene
            self.img = img[self.scene_corner_pix[1]:self.scene_corner_pix[1]+self.scene_size_pix[1],
                           self.scene_corner_pix[0]:self.scene_corner_pix[0]+self.scene_size_pix[0]]
        if self.cziscene_verbose:
            print('\tdone in %.4f s' % (time.time() - t, ))

        self.scene_loaded = True
        if self.cziscene_verbose:
            print('\tScene size is %d x %d' % (self.img.shape[0], self.img.shape[1]))

    def get_scene_info(self):
        """Access function for returning image and scene information.

        Returns:
          |  (m,n,nchan ndarray):  The scene image.
          |  (list of n,2 ndarray):  List of polygons defining slices in pixels.
          |  (list of n,2 ndarray):  List of polygons defining ROIs in pixels.
          |  (2, array):  Top-left coordinates of the polygon bounding box in pixels.
          |  (2, array):  Size of the polygon bounding box in pixels.

        .. note::

            Loads the metadata and scene or ribbon if not currently loaded.

        """
        if not self.scene_loaded: self.read_scene_image()
        return self.img, self.polygons_points, self.rois_points, self.box_corners_pix, self.box_sizes_pix

    def plot_scene(self, figno=1, doplots_ds=1, reduce=np.mean, interp_string='nearest', show=True):
        """Plot scene data using matplotlib.

        Kwargs:
          |  figno (int): Figure number to use.
          |  doplots_ds (int): Downsampling reduce factor before plotting.
          |  reduce (func): Function to use for block-reduce downsampling.
          |  interp_string (str): Interpolation string for matplotlib imshow.
          |  show (bool): Whether to show images or return immediately.

        """
        from matplotlib import pylab as pl
        import matplotlib.patches as patches
        import skimage.measure as measure

        if not self.scene_loaded: self.read_scene_image()

        if self.cziscene_verbose:
            print('\tblock reduce plot'); t = time.time()
        img_ds = measure.block_reduce(self.img, block_size=(doplots_ds, doplots_ds),
                                      func=reduce).astype(self.img.dtype) if doplots_ds > 1 else self.img
        if self.cziscene_verbose:
            print('\t\tdone in %.4f s' % (time.time() - t, ))

        pl.figure(figno);
        ax = pl.subplot(1,1,1)
        ax.imshow(img_ds,interpolation=interp_string, cmap='gray'); pl.axis('off')
        if self.ribbon >= 0:
            pl.title('Scene %d Ribbon %d' % (self.scene+1,self.ribbon+1))
        else:
            pl.title('Scene %d' % (self.scene+1,))
        for i in range(self.npolygons):
            poly = patches.Polygon(self.polygons_points[i]/doplots_ds,linewidth=1,edgecolor='r',facecolor='none')
            ax.add_patch(poly)
        for i in range(self.nROIs):
            poly = patches.Polygon(self.rois_points[i]/doplots_ds,linewidth=1,edgecolor='c',facecolor='none')
            ax.add_patch(poly)
        for i in range(self.nboxes):
            cnr = self.box_corners_pix[i,:]/doplots_ds; sz = self.box_sizes_pix[i,:]/doplots_ds
            rect = patches.Rectangle(cnr,sz[0],sz[1],linewidth=1,edgecolor='b',facecolor='none')
            ax.add_patch(rect)

        if show: pl.show()

    def export_tiff(self, save_tiff_ds=8, reduce=np.mean, fn=None):
        """Export scene image to tiff file.

        Kwargs:
          |  save_tiff_ds (int): Downsampling reduce factor before exporting.
          |  fn (str): Filename of tiff to export (default to filename provided in init)

        """
        import skimage.measure as measure
        import tifffile

        if fn is None: fn = self.tifffile_out
        # figure out BIG tiff
        if self.cziscene_verbose:
            print('Writing out imagej tiff'); t = time.time()
        img_ds = measure.block_reduce(self.img, block_size=(save_tiff_ds, save_tiff_ds), func=reduce)\
            .astype(self.img.dtype) if save_tiff_ds > 1 else self.img
        tifffile.imsave(fn,img_ds,imagej=True)
        if self.cziscene_verbose:
            print('\tdone in %.4f s' % (time.time() - t, ))

    @classmethod
    def readScene(cls, args):
        """Classmethod to create a CziScene object from args (argparse).

        Kwargs:
          |  args (argparse.Namespace): As returned by parse_args()

        """
        ret = cls(args.czi_filename[0], scene=args.scene[0], metafile_out=args.metafile_out[0],
                  tifffile_out=args.tifffile_out[0], verbose=args.cziscene_verbose)
        ret.read_scene_image()
        return ret

    @staticmethod
    def add_args(p):
        # adds arguments required for this object to specified ArgumentParser object
        p.add_argument('--czi-filename', nargs=1, type=str, default=[''], help='Input czi file')
        p.add_argument('--scene', nargs=1, type=int, default=[0], metavar='S', help='Specify scene to use in czi-file')
        p.add_argument('--metafile-out', nargs=1, type=str, default=[''], help='Meta file out (optional)')
        p.add_argument('--tifffile-out', nargs=1, type=str, default=[''], help='Tiff file out (optional)')
        p.add_argument('--cziscene-verbose', action='store_true', help='Verbose output')


