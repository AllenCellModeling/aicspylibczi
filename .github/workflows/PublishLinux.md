## Build Wheel for Manylinux2010

These instructions should build and publish the wheel

1) `docker pull pypywheels/manylinux2010-pypy_x86_64`
2) `docker run -ti --rm manylinux2010-pypy_x86_64`
3) update path with `export PATH=/opt/python/cp37-cp37m/bin:$PATH`
4) check cmake version if not current enough `pip install cmake==3.13.3`
5) clone the repo and checkout the submodules `git clone --recurse-submodules URL`
6) `pip install wheel`
7) `python setup.py bdist_wheel`
8) `pip install twine`
9) `pip install --user --upgrade twine`
10)  `twine upload --repository-url https://upload.pypi.org/legacy/ -u aicspypi -p '{PASSWORD}' dist/*`
11) check that the wheel name is manylinux2010 not just linux if it is not manually change it or pypi will reject it.

2019-12-16 
