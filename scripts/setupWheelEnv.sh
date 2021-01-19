#!/bin/bash

while getopts p:f:t: option
do
  case "${option}"
    in
      p) PYTHON_VERSION=${OPTARG};;
      f) ENV_NAME="jamies${OPTARG}";;
      t) MACOS_TARGET=${OPTARG};;
  esac
done

echo "PYTHON_VERSION=${PYTHON_VERSION}"
echo "ENV_NAME=${ENV_NAME}"
echo "MACOS_TARGET=${MACOS_TARGET}"

source ~/.bashrc
source ~/.bash_profile
export PATH=~/miniconda/bin:$PATH
~/miniconda/bin/conda create -y -n ${ENV_NAME} python=${PYTHON_VERSION}
conda activate ${ENV_NAME}
export SDKROOT=/Applications/Xcode_12.3.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.1.sdk
export MACOSX_DEPLOYMENT_TARGET=${MACOS_TARGET}
pip install wheel
python setup.py bdist_wheel
