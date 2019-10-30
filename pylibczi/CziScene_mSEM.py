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
from matplotlib.path import Path
from lxml import etree as etree

from .CziFile import CziFile
from . import types

class CziScene_mSEM(CziFile):
    """Zeiss CZI multibeam SEM scene image and metadata.

    Access functions allow for reading of czi metadata to extract information for a particular scene.

    Args:
      |  czi_filename (str): Filename of czifile to access scene information in.

    Kwargs:
      |  scene (int): The scene to load in czifile (starting at 1).
      |  ribbon (int): The ribbon to crop to (starting at 1). Negative value disables the ribbon cropping.
      |  metafile_out (str): Filename of xml file to export czi meta data to.
      |  verbose (bool): Print information and times during czi file access.

    .. note::

       Utilizes compiled wrapper to libCZI for accessing the CZI file.

    """

    # how many calibration markers to read in, this should essentially be a constant
    nmarkers = 3

    # xxx - likely this is a Zeiss bug,
    #   units for the scale in the xml file are not correct (says microns, given in meters)
    scale_units = 1e6

    xml_czi_paths = {
        'ScaleX':"/ImageDocument/Metadata/Scaling/Items/Distance[@Id = 'X']/Value",
        'ScaleY':"/ImageDocument/Metadata/Scaling/Items/Distance[@Id = 'Y']/Value",
        'Calibration':\
            '/ImageDocument/Metadata/Experiment/ExperimentBlocks/AcquisitionBlock/SubDimensionSetups/' +
                'CorrelativeSetup/HolderDocument/Calibration',
        'Scenes':'/ImageDocument/Metadata/Information/Image/Dimensions/S/Scenes',
        'CAT_Ribbon':"/ImageDocument/Metadata/MetadataNodes/MetadataNode/Layers/Layer[@Name = \"Cat_Ribbon\"]"+\
            '/Elements/Rectangle',
        'CAT_SectionPoints':\
            "/ImageDocument/Metadata/MetadataNodes/MetadataNode/Layers/Layer[@Name = \"CAT_Section\"]"+\
            "/Elements/Polygon",
        'CAT_ROIPoints':\
            "/ImageDocument/Metadata/MetadataNodes/MetadataNode/Layers/Layer[@Name = \"CAT_ROI\"]/Elements/Polygon",
        }

    xml_cz_paths = {
        'PolygonsPoints':"/GraphicsDocument/Elements/Polygon",
        }


    def __init__(self, czi_filename, scene=1, ribbon=0, metafile_out='', verbose=False):
        CziFile.__init__(self, czi_filename, metafile_out=metafile_out)
        self.scene, self.ribbon = scene-1, ribbon-1
        self.cziscene_verbose = verbose
        self.n_scenes = None


    def read_scene_meta(self):
        """Extract metadata from czifile relevant for scene.

        .. note::

           Sets class member variables from metadata pertaining to loading the initialized scene (or ribbon).

        """

        ### read and parse xml data from czi file
        self.read_meta()

        # how to get paths by searching for tags, for reference:
        #a = self.meta_root.findall('.//Polygon'); print('\n'.join([str(self.meta_root.getpath(x)) for x in a]))
        #a = self.meta_root.findall('.//Rectangle'); print('\n'.join([str(self.meta_root.getpath(x)) for x in a]))

        # get the pixel size
        self.scale = np.zeros((2,), dtype=np.double)
        self.scale[0] = float(self.meta_root.xpath(self.xml_czi_paths['ScaleX'])[0].text)*self.scale_units
        self.scale[1] = float(self.meta_root.xpath(self.xml_czi_paths['ScaleY'])[0].text)*self.scale_units

        # get the bounding box on the scene as specified in the meta data.
        # Could not find bounding box around all the scenes in the xml, which is the bounding box for the images.
        #   The bounding box for the images is not the same as the rectangle defined by the markers.
        #   So, load all the scene position and size information for calculating the bounding box.

        scenes = self.meta_root.xpath(self.xml_czi_paths['Scenes'])
        self.n_scenes = 0 if len(scenes) == 0 else len(scenes[0].findall('Scene'))
        assert( self.n_scenes > 0 ) # Error, Single Scene File. Use CziFile to access the data.
        scenes = scenes[0].findall('Scene')
        center_positions = np.zeros((self.n_scenes, 2), dtype=np.double)
        contour_sizes = np.zeros((self.n_scenes, 2), dtype=np.double)
        found = False
        for scene in scenes:
            i = int(scene.attrib['Index'])
            center_positions[i, :] = np.array([float(x) for x in scene.find('CenterPosition').text.split(',')])
            contour_sizes[i, :] = np.array([float(x) for x in scene.find('ContourSize').text.split(',')])
            found = (found or (i == self.scene))
        assert(found) # bad scene number
        center_position = center_positions[self.scene,:]
        contour_size = contour_sizes[self.scene,:]
        all_scenes_position = (center_positions - contour_sizes/2).min(axis=0)
        all_scenes_size = (center_positions + contour_sizes/2).max(axis=0)

        # get the marker positions
        marker_points = np.zeros((self.nmarkers,2),dtype=np.double) # xxx - do we need the z-position of the marker?
        markers = self.meta_root.xpath(self.xml_czi_paths['Calibration'])[0]
        for i in range(self.nmarkers):
            marker = markers.findall('.//Marker%d' % (i+1,))
            marker_points[i,0] = float(marker[0].findall('.//X')[0].text)
            marker_points[i,1] = float(marker[0].findall('.//Y')[0].text)

        ## Get the ribbon, slice (polygon) and roi points from the xml.
        # For all polygons, the rotation is not always specified in the xml. Default to zero.

        # get the ribbons around the polygons and rois
        ribbons = self.meta_root.xpath(self.xml_czi_paths['CAT_Ribbon'])
        nribbons = len(ribbons)
        ribbon_corners = np.zeros((nribbons,2),dtype=np.double)
        ribbon_sizes = np.zeros((nribbons,2),dtype=np.double)
        ribbon_rotations = np.zeros((nribbons,),dtype=np.double)
        for ribbon,i in zip(ribbons,range(nribbons)):
            ribbon_corners[i,0] = float(ribbon.findall('.//Geometry/Left')[0].text)
            ribbon_corners[i,1] = float(ribbon.findall('.//Geometry/Top')[0].text)
            ribbon_sizes[i,0] = float(ribbon.findall('.//Geometry/Width')[0].text)
            ribbon_sizes[i,1] = float(ribbon.findall('.//Geometry/Height')[0].text)
            rotation = ribbon.findall('.//Attributes/Rotation')
            if len(rotation) > 0:
                ribbon_rotations[i] = float(rotation[0].text)/180*np.pi

        # get the slice polygons and the ROI polygons
        polygons_points, polygons_rotation = \
            self._load_rois_from_xpath(self.meta_root.xpath(self.xml_czi_paths['CAT_SectionPoints']))
        rois_points, rois_rotation = \
            self._load_rois_from_xpath(self.meta_root.xpath(self.xml_czi_paths['CAT_ROIPoints']))

        ## calculate coordinate transformations and transform points to image coordinates

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

        ## transform polygons for ribbons, slices (polygons) and ROIs into scene space.

        # get the actual ribbon polygons by applying rotation.
        ribbon_points = [None]*nribbons
        for i in range(nribbons):
            ribbon_points[i] = np.concatenate((ribbon_corners[i,:][None,:],
                         (ribbon_corners[i,:] + [ribbon_sizes[i,0],0])[None,:],
                         (ribbon_corners[i,:] + ribbon_sizes[i,:])[None,:],
                         (ribbon_corners[i,:] + [0,ribbon_sizes[i,1]])[None,:]), axis=0)
        self.ribbon_points, self.ribbon_rotations = self._transform_polygons(ribbon_points, ribbon_rotations)
        self._adjust_ribbons_to_ribbon_points()

        # get the polygon and roi points relative to the specified scene or ribbon
        self.polygons_points, self.polygons_rotation = self._transform_polygons(polygons_points, polygons_rotation)
        self.rois_points, self.rois_rotation = self._transform_polygons(rois_points, rois_rotation)
        self.npolygons = len(self.polygons_points); self.nROIs = len(self.rois_points)

        # if a specific ribbon is requested, crop everything to the ribbon
        if self.ribbon >= 0:
            # this requires a special feature because in rare cases the ribbon was not placed properly.
            #   this assigns polygons to the nearest ribbon for those which are not overlapping wtih any ribbon.
            # assign each polygon to a ribbon based on proximity and
            #   then get bouding boxes of polygons assigned to the specified ribbon.
            bctrs = self.ribbon_corners + self.ribbon_sizes/2
            pmin, pmax = self._map_polys_to_ribbon_box(polygons_points, bctrs, self.ribbon_points)
            rmin, rmax = self._map_polys_to_ribbon_box(rois_points, bctrs, self.ribbon_points)

            # get the bounding box that encompasses the ribbon and the calculated polygon bounding boxes
            amin = np.vstack((pmin[None,:]-1, rmin[None,:]-1, self.ribbon_corners[self.ribbon,:][None,:])).min(0)
            amax = np.vstack((pmax[None,:]+1, rmax[None,:]+1, (self.ribbon_corners[self.ribbon,:] + \
                              self.ribbon_sizes[self.ribbon,:])[None,:])).max(0)

            self.scene_corner_pix += np.round(amin).astype(np.int64)
            self.scene_size_pix = np.round(amax-amin).astype(np.int64)
            # now there is only one ribbon for the scene
            self.nribbons = 1; self.ribbon_corners = np.zeros((1,2))
            self.ribbon_sizes = self.scene_size_pix[None,:]

            # correct all the polygons
            self.ribbon_points = [self.ribbon_points[self.ribbon] - amin]
            self.ribbon_rotations = self.ribbon_rotations[self.ribbon]
            for i in range(self.npolygons): self.polygons_points[i] -= amin
            for i in range(self.nROIs): self.rois_points[i] -= amin
        else:
            # trim the ribbons to those within the scene
            inscene = self._polygons_in_scene(self.ribbon_points, relative=True)
            self.ribbon_points = [x for x,y in zip(self.ribbon_points,inscene) if y]
            self.ribbon_rotations = self.ribbon_rotations[inscene]
            self._adjust_ribbons_to_ribbon_points()

        # trim the polygons and rois to those within the scene
        inscene = self._polygons_in_scene(self.polygons_points)
        self.polygons_points = [x for x,y in zip(self.polygons_points,inscene) if y]
        self.polygons_rotation = self.polygons_rotation[inscene]
        inscene = self._polygons_in_scene(self.rois_points)
        self.rois_points = [x for x,y in zip(self.rois_points,inscene) if y]
        self.rois_rotation = self.rois_rotation[inscene]
        self.npolygons = len(self.polygons_points); self.nROIs = len(self.rois_points)

        if self.cziscene_verbose:
            if self.ribbon >= 0:
                print( '%d polygons and %d ROIs are within scene %d, ribbon %d' % \
                      (self.npolygons, self.nROIs, self.scene+1, self.ribbon+1))
            else:
                print( '%d polygons, %d ROIs and %d ribbons are within scene %d' % \
                      (self.npolygons, self.nROIs, self.nribbons, self.scene+1))


    # START - helper functions for read_scene_meta

    def _adjust_ribbons_to_ribbon_points(self):
        self.nribbons = len(self.ribbon_points)
        self.ribbon_corners = np.zeros((self.nribbons,2), dtype=np.double)
        self.ribbon_sizes = np.zeros((self.nribbons,2), dtype=np.double)

        # re-adjust the ribbon corners and sizes as the bounding box of the ribbon polygons.
        for i in range(self.nribbons):
            self.ribbon_corners[i,:] = self.ribbon_points[i].min(0)
            self.ribbon_sizes[i,:] = self.ribbon_points[i].max(0) - self.ribbon_corners[i,:]

    def _map_polys_to_ribbon_box(self, polygons_points, box_centers, ribbon_points):
        npolygons = len(polygons_points)
        # calculate the distance from all polygon centers to all ribbon centers.
        pctrs = np.zeros((npolygons,2), dtype=np.double)
        for i in range(npolygons):
            p = polygons_points[i] - self.scene_corner_pix; m = p.min(0); pctrs[i,:] = m + (p.max(0) - m)/2
        # categorize each polygon as belonging to the closest ribbon center.
        d = scidist.cdist(box_centers,pctrs); pboxd = np.argmin(d, axis=0)
        # only use the distance categorization for polygons that are not within any ribbon.
        nribbons = len(ribbon_points)
        pbox = np.zeros((npolygons,), dtype=np.int64)
        ribbon_polygons = [None]*nribbons
        for i in range(nribbons):
            ribbon_polygons[i] = Path(ribbon_points[i])
        for j in range(npolygons):
            inside_ribbon = False
            for i in range(nribbons):
                if ribbon_polygons[i].contains_points(polygons_points[j]).any():
                    inside_ribbon = True; break
            if not inside_ribbon:
                print(j, polygons_points[j])
            pbox[j] = i if inside_ribbon else pboxd[j]
        # select all the polygons for the specified ribbon and get bounding box
        inds = np.nonzero(pbox == self.ribbon)[0]
        pmin = np.empty((2,), dtype=np.double); pmin.fill(np.inf)
        pmax = np.empty((2,), dtype=np.double); pmax.fill(-np.inf)
        for i in inds:
            p = polygons_points[i] - self.scene_corner_pix
            m = p.min(0); sel = (m < pmin); pmin[sel] = m[sel]
            m = p.max(0); sel = (m > pmax); pmax[sel] = m[sel]
        return pmin, pmax

    def _transform_polygons(self, polygons_points, polygons_rotation):
        npolygons = len(polygons_points)
        # points are are also relative to the scene bounding box, also get center of bounding box arond points.
        # polygons are rotated around the center of the bounding box of the polygon points.
        rpolygons_points = [None]*npolygons
        for i in range(npolygons):
            # correct for scene bounding box so points are relative to the scene itself
            rpolygons_points[i] = polygons_points[i] - self.scene_corner_pix

            if polygons_rotation[i] != 0:
                # create rotation matrix
                c, s = np.cos(polygons_rotation[i]), np.sin(polygons_rotation[i])
                R = np.array([[c, -s], [s, c]])

                # rotation centers calculated using the bounding boxes
                m = rpolygons_points[i].min(0); ctr = m + (rpolygons_points[i].max(0) - m)/2

                # center, rotate, then move back to center
                rpolygons_points[i] = np.dot(R, (rpolygons_points[i] - ctr).T).T + ctr
        return rpolygons_points, polygons_rotation

    def _polygons_in_scene(self, polygons_points, relative=False):
        npolygons = len(polygons_points)
        inscene = np.zeros((npolygons,), dtype=np.bool)
        for i in range(npolygons):
            smin = self.scene_corner_pix[None,:] if relative else np.zeros((1,2), dtype=np.int64)
            # determine if any part of the polygon is within the load scene
            inscene[i] = np.logical_and(polygons_points[i] >= smin,
                   polygons_points[i] < smin + self.scene_size_pix).all(1).any()
        return inscene

    def _load_rois_from_xpath(self, polygons):
        nROIs = len(polygons); rois_points = [None]*nROIs
        rois_rotation = np.zeros((nROIs,),dtype=np.double)
        for polygon,i in zip(polygons,range(nROIs)):
            rois_points[i] = np.array([[float(y) for y in x.split(',')] \
                           for x in polygon.findall('.//Points')[0].text.split(' ') if x])
            rotation = polygon.findall('.//Rotation')
            if len(rotation) > 0:
                rois_rotation[i] = float(rotation[0].text)/180*np.pi
        return rois_points, rois_rotation

    # END - helper functions for read_scene_meta


    """Load metadata from czfile.

    Args:

    Kwargs:

    Returns:

           Sets class member variables for rois from cz annotation file.
    """
    def load_cz_file_to_rois(self, cz_filename: types.FileLike):
        if not hasattr(self, 'scene_corner_pix'): self.read_scene_meta()
        self.roi_cz_meta_root = etree.parse(cz_filename)

        # get the ROI polygons
        rois_points, rois_rotation = \
            self._load_rois_from_xpath(self.roi_cz_meta_root.xpath(self.xml_cz_paths['PolygonsPoints']))

        self.rois_points, self.rois_rotation = self._transform_polygons(rois_points, rois_rotation)
        self.nROIs = len(self.rois_points)

        if self.cziscene_verbose:
            print('%d ROIs loaded from cz file: "%s"' % (self.nROIs, cz_filename))


    def read_scene_image(self, scale_factor=1.):
        """Load scene image and metadata from czifile.

        Args:

        Kwargs:

        Returns:
          |  (m,n,nchan ndarray):  The scene image.

        """
        if not hasattr(self, 'scene_corner_pix'): self.read_scene_meta()

        ### load the image data and crop to specified scene

        self.scale_factor = scale_factor
        if self.cziscene_verbose:
            print('Loading czi image for scene %d, scale factor %g' % (self.scene+1,self.scale_factor))
            t = time.time()

        # corner and size information of the mosaic from the czifile.
        mosaic_box = self.reader.mosaic_shape()

        # the bounding box calculated from the meta is calculated relative to all the scene bounding boxes,
        #   meaning that the origin is at the top-left of the top-left bounding box.
        # crop the region to be read if part of it is outside of the mosaic size.
        # if the whole region is somehow outside of bounds, allow the library to throw an exception.
        cnr = self.scene_corner_pix.copy(); cnr[cnr < 0] = 0
        sz = self.scene_size_pix.copy()
        tmp = cnr + sz - np.array([mosaic_box.w,mosaic_box.h])
        sel = (tmp > 0); sz[sel] -= tmp[sel]

        # for some reason the origin of mosaic files is not always zero, so move the origin to match the czifile.
        region = np.concatenate((cnr + np.array([mosaic_box.x,mosaic_box.y]), sz))
        # read the image and squeeze into two dimensions (channels not supported here).
        img = np.squeeze(self.read_mosaic(C=0, scale_factor=self.scale_factor, region=region))

        if self.cziscene_verbose:
            print('\tdone in %.4f s' % (time.time() - t, ))
            print('\tScene size is %d x %d' % (img.shape[0], img.shape[1]))

        return img


    def plot_scene(self, figno=1, scale_factor=1., interp_string='nearest', show=True):
        """Plot scene data using matplotlib.

        Kwargs:
          |  figno (int): Figure number to use.
          |  scale_factor (float): amount to scale the data by
               0.1 would mean an image 1/10 the height and width of native
          |  interp_string (str): Interpolation string for matplotlib imshow.
          |  show (bool): Whether to show images or return immediately.

        """
        from matplotlib import pylab as pl
        import matplotlib.patches as patches

        img = self.read_scene_image(scale_factor=scale_factor)

        pl.figure(figno);
        ax = pl.subplot(1,1,1)
        ax.imshow(img,interpolation=interp_string, cmap='gray'); pl.axis('off')
        sf = self.scale_factor
        if self.ribbon >= 0:
            pl.title('Scene %d Ribbon %d' % (self.scene+1,self.ribbon+1))
        else:
            pl.title('Scene %d' % (self.scene+1,))
        for i in range(self.npolygons):
            poly = patches.Polygon(self.polygons_points[i]*sf,linewidth=1,edgecolor='r',facecolor='none')
            ax.add_patch(poly)
        for i in range(self.nROIs):
            poly = patches.Polygon(self.rois_points[i]*sf,linewidth=1,edgecolor='c',facecolor='none')
            ax.add_patch(poly)
        for i in range(self.nribbons):
            cnr = self.ribbon_corners[i,:]*sf; sz = self.ribbon_sizes[i,:]*sf
            rect = patches.Rectangle(cnr,sz[0],sz[1],linewidth=1,edgecolor='b',facecolor='none',linestyle='--')
            ax.add_patch(rect)
            poly = patches.Polygon(self.ribbon_points[i]*sf,linewidth=1,edgecolor='b',facecolor='none')
            ax.add_patch(poly)

        if show: pl.show()

