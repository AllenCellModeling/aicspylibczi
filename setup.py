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

from setuptools import setup, Extension
import sys, os
import glob
import numpy
import sysconfig
import platform
import subprocess

# several platform specifc build options
platform_ = platform.system()
architecture = platform.architecture()

# to statically link libCZI
build_static = (platform_ != 'Windows')

# libczi cloned as submodule, get paths to library and build locations.
script_dir = os.path.dirname(os.path.abspath(__file__))
libczi_dir = os.path.join(script_dir, 'libCZI')
build_temp = os.path.join(libczi_dir,'build')
include_libCZI = os.path.join(libczi_dir, 'Src')
lib_libCZI = os.path.join(build_temp, 'Src', 'libCZI')
lib_JxrDecode = os.path.join(build_temp, 'Src', 'JxrDecode')
if platform_ == 'Windows':
    if build_static:
        # xxx - does not work, not sure why
        #   copy libCZI dll using data_files instead
        libCZI_win_release = 'static\ Release'
    else:
        libCZI_win_release = 'Release'
    win_arch = 'x64' if architecture[0]=='64bit' else 'x86'
    lib_libCZI = os.path.join(lib_libCZI, libCZI_win_release)
    lib_JxrDecode = os.path.join(lib_JxrDecode, libCZI_win_release)


def build_libCZI():
    env = os.environ.copy()
    cmake_args = []
    build_args = []
    if platform_ == 'Windows':
        cmake_args += ['-DCMAKE_GENERATOR_PLATFORM=' + win_arch]
        build_args += ['--config', libCZI_win_release]
    else:
        cmake_args += ['-DCMAKE_BUILD_TYPE:STRING=Release']
    if not os.path.exists(build_temp):
        os.makedirs(build_temp)
    def run_cmake(cmake_exe):
        config_cmd_list = [cmake_exe, libczi_dir] + cmake_args
        build_cmd_list = [cmake_exe, '--build', '.'] + build_args
        if platform_ == 'Windows':
            print(subprocess.list2cmdline(config_cmd_list))
            print(subprocess.list2cmdline(build_cmd_list))
        subprocess.check_call(config_cmd_list, cwd=build_temp, env=env)
        subprocess.check_call(build_cmd_list, cwd=build_temp, env=env)
    try:
        # try to use the pip installed cmake first
        run_cmake(os.path.join(os.path.dirname(sys.executable), 'cmake'))
    except OSError:
        # xxx - anaconda windows pip install cmake goes into a Scripts dir
        #   better option here?
        run_cmake('cmake')
build_libCZI()


def safe_get_env_var_list(var):
    vlist = sysconfig.get_config_var(var)
    return ([] if vlist is None else vlist.split())

# platform specific compiler / linker options
extra_compile_args = safe_get_env_var_list('CFLAGS')
extra_link_args = safe_get_env_var_list('LDFLAGS')
if platform_ == 'Windows':
    extra_compile_args += safe_get_env_var_list('CL')
    extra_compile_args += safe_get_env_var_list('_CL_')
    extra_link_args += safe_get_env_var_list('LINK')
    extra_link_args += safe_get_env_var_list('_LINK_')
    extra_compile_args += ['/Ox']
else:
    extra_compile_args += ["-std=c++11", "-Wall", "-O3"]
    if platform_ == 'Linux':
        extra_compile_args += ["-fPIC"]
        if build_static:
            # need to link with g++ linker for static libstdc++ to work
            os.environ["LDSHARED"] = os.environ["CXX"] if 'CXX' in os.environ else 'g++'
            extra_link_args += ['-static-libstdc++', '-shared']
            #extra_link_args += ["-Wl,--no-undefined"] # will not work with manylinux
    elif platform_ == 'Darwin':
        mac_ver = platform.mac_ver()[0] # xxx - how to know min mac version?
        extra_compile_args += ["-stdlib=libc++", "-mmacosx-version-min="+mac_ver]
    extra_link_args += extra_compile_args

include_dirs = [numpy.get_include(), include_libCZI]

static_libraries = []
static_lib_dirs = []
libraries = []
library_dirs = []
extra_objects = []
if build_static:
    static_libraries += ['libCZIStatic', 'JxrDecodeStatic']
    static_lib_dirs += [lib_libCZI, lib_JxrDecode]
else:
    libraries += ['libCZI']
    library_dirs += [lib_libCZI]

# second answer at
# https://stackoverflow.com/questions/4597228/how-to-statically-link-a-library-when-compiling-a-python-module-extension
if platform_ == 'Windows':
    libraries.extend(static_libraries)
    library_dirs.extend(static_lib_dirs)
else: # POSIX
    extra_objects += ['{}/lib{}.a'.format(d, l) for d,l in zip(static_lib_dirs, static_libraries)]
    include_dirs.append('/usr/local/include')
    library_dirs.append('/usr/local/lib')

sources = ['_pylibczi.cpp']
version = open("pylibczi/_version.py").readlines()[-1].split()[-1].strip("\"'")

module1 = Extension('_pylibczi',
                    define_macros = [('PYLIBCZI_VERSION', version),],
                    include_dirs = include_dirs,
                    libraries = libraries,
                    library_dirs = library_dirs,
                    sources = [os.path.join('_pylibczi',x) for x in sources],
                    extra_compile_args=extra_compile_args,
                    extra_link_args=extra_link_args,
                    language='c++11',
                    extra_objects=extra_objects,
                    )


data_files = []
if not build_static:
    data_files += glob.glob(os.path.join(lib_libCZI,'*.so'))
    data_files += glob.glob(os.path.join(lib_libCZI,'*.dylib'))
    data_files += glob.glob(os.path.join(lib_libCZI,'*.dll'))

setup (name = 'pylibczi',
       version=version,
       description = 'Python module utilizing libCZI for reading Zeiss CZI files.',
       author = 'Paul Watkins',
       author_email = 'pwatkins@gmail.com',
       url = 'https://github.com/elhuhdron/pylibczi',
       long_description = '''
Python module to expose libCZI functionality for reading (subset of) Zeiss CZI files and meta-data.
''',
       ext_modules = [module1],
       packages = ['pylibczi'],
       data_files = data_files,
       install_requires=['scikit-image', 'matplotlib', 'tifffile', 'scipy', 'numpy', 'lxml', 'cmake'],
       )
