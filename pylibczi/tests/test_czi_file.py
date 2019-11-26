import pytest
from pylibczi import CziFile
from pathlib import Path


@pytest.mark.parametrize("as_string", [
    pytest.param(True, marks=pytest.mark.raises(exception=IsADirectoryError)),
    pytest.param(False, marks=pytest.mark.raises(exception=IsADirectoryError))
])
def test_is_a_directory(data_dir, as_string):
    infile = data_dir
    if as_string:
        infile = str(data_dir)
    CziFile(infile)


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
    czi = CziFile(str(data_dir / fname))
    czi.read_meta()
    meta = czi.meta_root
    vs = meta.find(xp_query)
    assert int(vs.text) == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (1, 325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (3, 3, 5, 325, 475)),
])
def test_read_image_from_istream(data_dir, fname, expected):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        data = czi.read_image()
        assert data[0].shape == expected


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
    czi = CziFile(str(data_dir / fname))
    #this should segfault on windows


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', [0, 0, -1, -1]),
    ('s_3_t_1_c_3_z_5.czi', [0, 0, -1, -1]),
])
def test_mosaic_size(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    ans = czi.read_mosaic_size()
    assert ans.x == expected[0]
    assert ans.y == expected[1]
    assert ans.w == expected[2]
    assert ans.h == expected[3]


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (325, 475)),
])
def test_read_image(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    img, shp = czi.read_image(C=0, B=0)
    assert img.shape[-2] == expected[0]
    assert img.shape[-1] == expected[1]


@pytest.mark.parametrize("fname, exp_str, exp_dict", [
    ('s_1_t_1_c_1_z_1.czi', "BC", {'B': (0, 1), 'C': (0, 1)}),
    ('s_3_t_1_c_3_z_5.czi', "BSCZ", {'B': (0, 1), 'C': (0, 3), 'Z': (0, 5), 'S': (0, 3)}),
])
def test_read_image_two(data_dir, fname, exp_str, exp_dict):
    czi = CziFile(str(data_dir / fname))
    dims = czi.dims
    dim_dict = czi.dims_shape()
    assert dims == exp_str
    assert dim_dict == exp_dict


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
    ('s_1_t_1_c_1_z_1.czi', {'B': (0, 1), 'C': (0, 1)}),  # single dims except for C are dropped (S/T dropped) B is always used
    ('s_3_t_1_c_3_z_5.czi', {'B': (0, 1), 'S': (0, 3), 'C': (0, 3), 'Z': (0, 5)}),
])
def test_image_shape(data_dir, fname, expects):
    with open(data_dir / fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        shape = czi.dims_shape()
    for key in expects.keys():
        assert shape[key] == expects[key]

# I'm waiting for a small mosaic file then I can add an equivalent test back in
# def test_mosaic_image():
#     pTwo = Path('~/Data/20190618_CL001_HB01_Rescan_002.czi').expanduser()
#     czi = CziFile(str(pTwo))
#     sze = czi.read_mosaic_size()
#     assert sze.w == 69986
#     assert sze.h == 62649
#     img = czi.read_mosaic(scale_factor=0.1, C=1)
#     assert img.shape[0] == 1
