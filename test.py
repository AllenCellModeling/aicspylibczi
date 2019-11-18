import pylibczi
import pathlib

print("starting Brute Force Test")

pth = pathlib.Path('./pylibczi/tests/resources/s_1_t_1_c_1_z_1.czi').resolve()

czi = pylibczi.CziFile(pth)
czi = pylibczi.CziFile(pth)

print("survived continuing to get metadata")


#mta = czi.read_subblock_metadata()

#print(mta)


