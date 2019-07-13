import argparse
from pylibczi.CziScene import CziScene

parser = argparse.ArgumentParser(description='Scene image and meta data for Zeiss czi files',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
CziScene.add_args(parser)
args = parser.parse_args()

scene = CziScene.readScene(args)
if scene.tifffile_out:
    scene.export_tiff()
scene.plot_scene()
