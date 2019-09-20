import pytest
from pylibczi import CziScene
from pylibczi.exceptions import CziFileNotMultiSceneError


@pytest.mark.parametrize("fname, xp_query, expected", [
#    pytest.param('s_1_t_1_c_1_z_1.czi', None, None, marks=pytest.mark.raises(exception=CziFileNotMultiSceneError)),
#    ('s_3_t_1_c_3_z_5.czi', ".//SizeS", 3),
#    pytest.param('s_3_t_1_c_3_z_5.czi', ".//SizeT", 1, marks=pytest.mark.raises(exception=AttributeError)),
    ('20190425_S08_001-04-Scene-3-P4-B03.czi', ".//SizeT", 3),
#    ('s_3_t_1_c_3_z_5.czi', ".//SizeZ", 5),
])
def test_metadata(data_dir, fname, xp_query, expected):
    czi = CziScene(str(data_dir / fname), scene=1)
    czi.read_scene_meta()
    meta = czi.meta_root
    vs = meta.find(xp_query)
    assert int(vs.text) == expected


@pytest.mark.parametrize("fname, expected", [
    ('s_1_t_1_c_1_z_1.czi', (325, 475)),
    ('s_3_t_1_c_3_z_5.czi', (4029, 5476)),
])
def test_read_image(data_dir, fname, expected):
    czi = CziScene(str(data_dir / fname), scene=1)
    img = czi.read_image()
    print(fname, " => ", img.shape)
    assert img.shape == expected