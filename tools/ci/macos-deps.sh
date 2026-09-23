#!/bin/bash
#
# Builds the libraries that the macOS release build links statically
# (Boost, fmt and spdlog) for one architecture and minimum macOS version.
# Homebrew's builds can't be used for this: they only run on the macOS of
# the machine they were built for, and Homebrew no longer builds for Intel.
#
# Usage: macos-deps.sh <prefix>
# Environment: MACOSX_DEPLOYMENT_TARGET (default 13.0), ARCH (default x86_64)

set -euo pipefail

PREFIX=$1
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-13.0}
ARCH=${ARCH:-x86_64}

BOOST_VERSION=1.92.0
BOOST_SHA256=5c1d40cb8e19adbf740a4ec2da35b3e58f3f5804b1dce44deb53df72193cbc6c
# The same versions CMakeLists.txt fetches when it finds none installed.
FMT_TAG=12.1.0
SPDLOG_TAG=v1.17.0

BASEDIR=$(cd "$(dirname "$0")" && pwd)
WORK=$(mktemp -d)
JOBS=$(sysctl -n hw.ncpu)
FLAGS="-arch ${ARCH} -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}"

# Boost: all headers, and the three compiled libraries CMakeLists.txt asks for.
boost_name=boost_${BOOST_VERSION//./_}
curl -fsSL --retry 3 -o "${WORK}/${boost_name}.tar.bz2" \
    "https://archives.boost.io/release/${BOOST_VERSION}/source/${boost_name}.tar.bz2"
echo "${BOOST_SHA256}  ${WORK}/${boost_name}.tar.bz2" | shasum -a 256 -c -
tar -xjf "${WORK}/${boost_name}.tar.bz2" -C "${WORK}"
(
    cd "${WORK}/${boost_name}"
    ./bootstrap.sh
    ./b2 -j"${JOBS}" --prefix="${PREFIX}" \
        --with-date_time --with-program_options --with-serialization \
        link=static runtime-link=shared threading=multi variant=release \
        cflags="${FLAGS}" cxxflags="${FLAGS}" linkflags="${FLAGS}" \
        install
)

cmake_dependency() { # <git url> <tag> [cmake arguments...]
    local url=$1 tag=$2
    shift 2
    local name
    name=$(basename "${url}" .git)
    git clone --depth 1 --branch "${tag}" "${url}" "${WORK}/${name}"
    cmake -S "${WORK}/${name}" -B "${WORK}/${name}-build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
        -DCMAKE_PREFIX_PATH="${PREFIX}" \
        -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
        -DBUILD_SHARED_LIBS=OFF \
        "$@"
    cmake --build "${WORK}/${name}-build"
    cmake --install "${WORK}/${name}-build"
}

cmake_dependency https://github.com/fmtlib/fmt.git "${FMT_TAG}" -DFMT_DOC=OFF -DFMT_TEST=OFF
cmake_dependency https://github.com/gabime/spdlog.git "${SPDLOG_TAG}" \
    -DSPDLOG_FMT_EXTERNAL=ON -DSPDLOG_BUILD_EXAMPLE=OFF

rm -rf "${WORK}"

python3 "${BASEDIR}/check-macos-binaries.py" --arch "${ARCH}" \
    --min-macos "${MACOSX_DEPLOYMENT_TARGET}" "${PREFIX}/lib"
