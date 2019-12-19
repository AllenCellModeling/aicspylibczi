import io
import numpy as np
import pytest

from aicspylibczi import CziFile
from _aicspylibczi import PylibCZI_CDimCoordinatesOverspecifiedException


@pytest.mark.parametrize("as_string", [
    pytest.param(True, marks=pytest.mark.raises(exception=IsADirectoryError)),
    pytest.param(False, marks=pytest.mark.raises(exception=IsADirectoryError))
])
def test_is_a_directory(data_dir, as_string):
    infile = data_dir
    if as_string:
        infile = str(data_dir)
    CziFile(infile)


@pytest.mark.parametrize("in_, out_", [
    pytest.param(float(10), False, marks=pytest.mark.raises(exception=TypeError)),
    (io.BytesIO(b"thisisatestletsseewhathappens"), io.BytesIO(b"thisisatestletsseewhathappens")),
    (b"thisisatestletsseewhathappens", io.BytesIO(b"thisisatestletsseewhathappens")),
    (np.zeros(5), np.zeros(5))
])
def test_conversion_types(in_, out_):
    ans = CziFile.convert_to_buffer(in_)
    assert ans.__class__ == out_.__class__


@pytest.mark.parametrize("fname, xp_query, expected", [
    pytest.param('s_1_t_1_c_1_z_1.czi', ".//SizeS", 1, marks=pytest.mark.raises(exception=AttributeError)),
    pytest.param('s_1_t_1_c_1_z_1.czi', ".//SizeZ", 1, marks=pytest.mark.raises(exception=AttributeError)),
    pytest.param('s_1_t_1_c_1_z_1.czi', ".//SizeT", 1, marks=pytest.mark.raises(exception=AttributeError)),
    ('s_1_t_1_c_1_z_1.czi', ".//SizeC", 1),
    ('s_1_t_1_c_1_z_1.czi', ".//SizeY", 325),
    ('s_1_t_1_c_1_z_1.czi', ".//SizeX", 475),
    ('s_3_t_1_c_3_z_5.czi', ".//SizeS", 3),
    pytest.param('s_3_t_1_c_3_z_5.czi', ".//SizeT", 1, marks=pytest.mark.raises(exception=AttributeError)),
    ('s_3_t_1_c_3_z_5.czi', ".//SizeC", 3),
    ('s_3_t_1_c_3_z_5.czi', ".//SizeZ", 5),
])
def test_metadata(data_dir, fname, xp_query, expected):
    czi = CziFile(str(data_dir / fname), metafile_out="tmp.txt")
    meta = czi.meta
    vs = meta.find(xp_query)
    assert int(vs.text) == expected
    meta = czi.meta
    vs = meta.find(xp_query)
    assert int(vs.text) == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (1, 1, 325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (1, 3, 3, 5, 325, 475)),
])
def test_read_image_from_istream(data_dir, fname, expected):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        data = czi.read_image()
        assert data[0].shape == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (1, 1, 325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (1, 3, 3, 5, 325, 475)),
    ('mosaic_test.czi', (1, 1, 1, 1, 2, 624, 924)),  # S T C Z M Y X
])
def test_read_dims_sizes(data_dir, fname, expected):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        data = czi.size
        assert data == expected


@pytest.mark.parametrize("fname, idx, expected", [
    ('s_1_t_1_c_1_z_1.czi', -1, (325, 475)),
    ('s_1_t_1_c_1_z_1.czi', 0, (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', -1, (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', 0, (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', 1, (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', 2, (325, 475)),
])
def test_scene_height_width(data_dir, fname, idx, expected):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        data = czi.scene_height_by_width(idx)
        assert data == expected


@pytest.mark.parametrize("fname, idx, expected", [
    ('s_1_t_1_c_1_z_1.czi', -1, (39856, 39272, 475, 325)),
    ('s_1_t_1_c_1_z_1.czi', 0, (39856, 39272,  475, 325)),
    ('s_3_t_1_c_3_z_5.czi', -1, (39850, 35568, 475, 325)),
    ('s_3_t_1_c_3_z_5.czi', 0, (39850, 35568, 475, 325)),
    ('s_3_t_1_c_3_z_5.czi', 1, (39850, 35568, 475, 325)),
    ('s_3_t_1_c_3_z_5.czi', 2, (39850, 39272, 475, 325)),
])
def test_scene_bbox(data_dir, fname, idx, expected):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        data = czi.scene_bounding_box(idx)
        assert data == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', False),
    ('s_3_t_1_c_3_z_5.czi', False),
])
def test_is_mosaic(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    assert czi.is_mosaic() == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (False)),
    ('s_3_t_1_c_3_z_5.czi', (False)),
])
def test_destructor(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    czi = CziFile(str(data_dir / fname))  # NOQA F841


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (0, 0, -1, -1)),
    ('s_3_t_1_c_3_z_5.czi', (0, 0, -1, -1)),
    ('mosaic_test.czi', (0, 0, 1756, 624)),  # it's not 2*X because they overlap
])
def test_mosaic_size(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    ans = czi.read_mosaic_size()
    assert ans == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (1, 1, 325, 475)),  # B C Y X
    ('s_3_t_1_c_3_z_5.czi', (1, 3, 3, 5, 325, 475)),  # B S C Z Y X
    ('mosaic_test.czi', (1, 1, 1, 1, 2, 624, 924)),  # S T C Z M Y X
    ('RGB-8bit.czi', (1, 1, 3, 624, 924)),  # B T C Y X
])
def test_read_image(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    img, shp = czi.read_image()
    assert img.shape == expected


@pytest.mark.parametrize("fname, exp_str, exp_dict", [
    ('s_1_t_1_c_1_z_1.czi', "BCYX", {'B': (0, 0), 'C': (0, 0), 'X': (0, 474), 'Y': (0, 324)}),
    ('s_3_t_1_c_3_z_5.czi', "BSCZYX", {'B': (0, 0), 'C': (0, 2), 'X': (0, 474), 'Y': (0, 324),
                                       'S': (0, 2), 'Z': (0, 4)}),
    ('mosaic_test.czi', "STCZMYX", {'S': (0, 0), 'T': (0, 0), 'C': (0, 0), 'Z': (0, 0),
                                    'M': (0, 1), 'Y': (0, 623), 'X': (0, 923)}),

])
def test_read_image_two(data_dir, fname, exp_str, exp_dict):
    czi = CziFile(str(data_dir / fname))
    dims = czi.dims
    dim_dict = czi.dims_shape()
    assert dims == exp_str
    assert dim_dict == exp_dict


@pytest.mark.parametrize("fname", [
    pytest.param('s_1_t_1_c_1_z_1.czi', marks=pytest.mark.raises(exception=PylibCZI_CDimCoordinatesOverspecifiedException)),
])
def test_read_image_mosaic(data_dir, fname):
    czi = CziFile(str(data_dir / fname))
    czi.read_image(M=0)
    assert True


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', "<METADATA><Tags><AcquisitionTime>2019-06-27T18:33:41.1154211Z</AcquisitionTime>"),
    ('s_3_t_1_c_3_z_5.czi', "<METADATA><Tags><AcquisitionTime>2019-06-27T18:39:26.6459707Z</AcquisitionTime>"),
])
def test_read_subblock_meta(data_dir, fname, expected):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        data = czi.read_subblock_metadata()
        assert expected in data[0][1]


@pytest.mark.parametrize("fname, expects", [
    ('s_1_t_1_c_1_z_1.czi', {'B': (0, 0), 'C': (0, 0), 'X': (0, 474), 'Y': (0, 324)}),
    ('s_3_t_1_c_3_z_5.czi', {'B': (0, 0), 'C': (0, 2), 'X': (0, 474), 'Y': (0, 324), 'S': (0, 2), 'Z': (0, 4)}),
])
def test_image_shape(data_dir, fname, expects):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        shape = czi.dims_shape()
    for key in expects.keys():
        assert shape[key] == expects[key]


@pytest.mark.parametrize("fname, expects", [
    ('mosaic_test.czi', {'B': (0, 0), 'C': (0, 0), 'X': (0, 474), 'Y': (0, 324)}),
])
def test_mosaic_image(data_dir, fname, expects):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        sze = czi.read_mosaic_size()
        assert sze[2] == 1756
        assert sze[3] == 624
        img = czi.read_mosaic(scale_factor=0.1, C=0)
    assert img.shape[0] == 1


@pytest.mark.parametrize("fname, expects", [
    ('mosaic_test.czi', (1, 1, int(624/2), int(1756/2))),
])
def test_mosaic_image_two(data_dir, fname, expects):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        sze = czi.read_mosaic_size()
        rgion = (sze[0], sze[1], int(sze[2]/2), int(sze[3]/2))
        img = czi.read_mosaic(region=rgion, C=0, M=0)
    assert img.shape == expects
