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


@pytest.mark.parametrize("fname, expects", [
    ('s_1_t_1_c_1_z_1.czi', {'B': 1, 'C': 1}),  # single dims except for C are dropped (S/T dropped) B is always used
    ('s_3_t_1_c_3_z_5.czi', {'B': 1, 'S': 3, 'C': 3, 'Z': 5}),
    ('20190425_S08_001-04-Scene-3-P4-B03.czi', {'B': 1, 'S': 1, 'T': 61, 'C': 2, 'Z': 70})
])
def test_image_shape(data_dir, fname, expects):
    with open(data_dir/fname, 'rb') as fp:
        czi = CziFile(czi_filename=fp)
        shape = czi.dims()
    for key in expects.keys():
        assert shape[key] == expects[key]
