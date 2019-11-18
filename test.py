import pylibczi
import pathlib

print("starting Brute Force Test")

pth = pathlib.Path('./pylibczi/tests/resources/s_1_t_1_c_1_z_1.czi').resolve()
fp = open(pth, "rb")
czi = pylibczi.CziFile(fp)
print("closing file")
czi = pylibczi.CziFile(fp)


print("survived")




