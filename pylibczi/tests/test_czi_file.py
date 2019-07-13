import pytest
from pylibczi import CziFile


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
    ('s_1_t_1_c_1_z_1.czi', (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (4029, 5476)),
])
def test_read_image(data_dir, fname, expected):
    czi = CziFile(str(data_dir / fname))
    img = czi.read_image()
    print(fname, " => ", img.shape)
    assert img.shape == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (4029, 5476)),
])
def test_read_image_from_istream(data_dir, fname, expected):
    with open(data_dir/fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        img = czi.read_image()
        print(fname, " => ", img.shape)
    assert img.shape == expected
