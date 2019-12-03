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

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

import os
import platform
import re
import subprocess
import sys
from distutils.version import LooseVersion


with open("README.md") as readme_file:
    readme = readme_file.read()

requirements = [
    "numpy>=1.14.1",
    "lxml",
    "scikit-image",
]

test_requirements = [
    "codecov",
    "flake8",
    "pytest",
    "pytest-cov",
    "pytest-raises",
    "pytest-xdist",
    "Sphinx>=2.1.0b1",
    "sphinx_rtd_theme>=0.1.2",
]

dev_requirements = [
    "bumpversion>=0.5.3",
    "coverage>=5.0a4",
    "flake8>=3.7.7",
    "ipython>=7.5.0",
    "m2r>=0.2.1",
    "pytest>=4.3.0",
    "pytest-cov==2.6.1",
    "pytest-raises>=0.10",
    "pytest-runner>=4.4",
    "Sphinx>=2.1.0b1",
    "sphinx_rtd_theme>=0.1.2",
    "breathe",
    "tox>=3.5.2",
    "twine>=1.13.0",
    "wheel>=0.33.1",
    "cmake",
]

setup_requirements = [
    "pytest-runner",
]

interactive_requirements = [
    "altair",
    "jupyterlab",
    "matplotlib",
    "pillow",
]

extra_requirements = {
    "test": test_requirements,
    "dev": dev_requirements,
    "setup": setup_requirements,
    "interactive": interactive_requirements,
    "all": [
        *requirements,
        *test_requirements,
        *setup_requirements,
        *dev_requirements,
        *interactive_requirements
    ]
}


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.', '--target', '_pylibczi'] + build_args, cwd=self.build_temp)


setup(
    name='pylibczi',
    # Do not edit this string manually, always use bumpversion
    # Details in CONTRIBUTING.md
    version='2.1.0',
    author='Paul Watkins, Jamie Sherman',
    author_email='pwatkins@gmail.com, jamies@alleninstitute.org',
    description='A python module and a python extension for Zeiss (CZI/ZISRAW) microscopy files.',
    long_description=readme,
    long_description_content_type="text/markdown",
    include_package_data=True,
    keywords="aicsimageio, allen cell, imaging, computational biology",
    ext_modules=[CMakeExtension('_pylibczi')],
    packages=['pylibczi'],
    cmdclass=dict(build_ext=CMakeBuild),
    install_requires=requirements,
    setup_requires=setup_requirements,
    test_suite='pylibczi/tests',
    tests_require=test_requirements,
    extras_require=extra_requirements,
    url="https://github.com/AllenCellModeling/pylibczi",
    zip_safe=False,
)
