#!/bin/bash
#
# Builds the macOS release app and disk image for one architecture and
# minimum macOS version, then checks the result: the checks run the app
# from the disk image, because the build machine can't show whether the
# app starts on the older Macs it targets any other way.
#
# Environment:
#   QT_ROOT_DIR               Qt for macOS (the official build, for its minimum macOS)
#   DEPS_PREFIX               the libraries macos-deps.sh installed
#   MACOSX_DEPLOYMENT_TARGET  default 13.0
#   ARCH                      default x86_64
# plus what config.sh reads (WORKRAVE_ENV and friends).

set -e

BASEDIR=$(cd "$(dirname "$0")" && pwd)
# config.sh reads variables that may be unset, so source it before set -u.
source "${BASEDIR}/config.sh"

set -uo pipefail
shopt -s nullglob

: "${QT_ROOT_DIR:?}" "${DEPS_PREFIX:?}"
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-13.0}
ARCH=${ARCH:-x86_64}

set -x

cmake -S "${SOURCES_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${OUTPUT_DIR}" \
    -DCMAKE_PREFIX_PATH="${QT_ROOT_DIR};${DEPS_PREFIX}" \
    -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
    -DBoost_USE_STATIC_LIBS=ON \
    -DRPC_CODEGEN=OFF \
    -DWITH_UI=Qt \
    -DWITH_INDICATOR=OFF -DWITH_GSTREAMER=OFF -DWITH_PULSE=OFF -DWITH_GNOME_CLASSIC_PANEL=OFF

# The dmg target installs the app bundle (macdeployqt, then an ad-hoc
# signature) and packs it into a disk image.
ninja -C "${BUILD_DIR}"
ninja -C "${BUILD_DIR}" dmg

app="${OUTPUT_DIR}/stopme.app"
dmgs=("${OUTPUT_DIR}"/*.dmg)
test -d "${app}"
test ${#dmgs[@]} -eq 1
dmg=${dmgs[0]}

/usr/libexec/PlistBuddy -c Print "${app}/Contents/Info.plist"

# Everything in the bundle runs on the target macOS and architecture and
# loads nothing from outside the bundle but the system.
python3 "${BASEDIR}/check-macos-binaries.py" --bundle --arch "${ARCH}" \
    --min-macos "${MACOSX_DEPLOYMENT_TARGET}" "${app}"

# A bundle that no longer matches its signature is "damaged" to Gatekeeper.
codesign --verify --deep --strict --verbose=2 "${app}"

# Start the app from the disk image and make sure it keeps running.
smoke=$(mktemp -d)
mount_point="${smoke}/volume"
mkdir "${mount_point}"
hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "${mount_point}" "${dmg}"
ditto "${mount_point}/stopme.app" "${smoke}/stopme.app"
hdiutil detach "${mount_point}" || hdiutil detach -force "${mount_point}"
codesign --verify --deep --strict "${smoke}/stopme.app"

executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "${smoke}/stopme.app/Contents/Info.plist")
"${smoke}/stopme.app/Contents/MacOS/${executable}" >"${smoke}/stopme.log" 2>&1 &
pid=$!
sleep 30
if kill -0 "${pid}"; then
    running=1
    kill "${pid}"
    sleep 5
    kill -9 "${pid}" 2>/dev/null || true
    wait "${pid}" || true
else
    running=0
fi
set +x
echo "---- stopme output"
cat "${smoke}/stopme.log"
logs=("${smoke}/stopme.log" "${HOME}/Library/Logs/stopme/"*.log)
for log in "${logs[@]:1}"; do
    echo "---- ${log}"
    cat "${log}"
done
echo "----"
set -x
if [[ ${running} != 1 ]]; then
    echo "error: stopme exited within 30 seconds of starting" >&2
    exit 1
fi
if grep -E 'Could not find the Qt platform plugin|Library not loaded|failed to load component|is not installed|is not a type|Type [A-Za-z0-9_]+ unavailable' "${logs[@]}"; then
    echo "error: stopme reported errors while starting" >&2
    exit 1
fi

# Deploy under the same naming scheme as the other downloads.
if [[ -z "${WORKRAVE_RELEASE:-}" ]]; then
    postfix=${WORKRAVE_LONG_GIT_VERSION}-${WORKRAVE_BUILD_DATE}
else
    postfix=${WORKRAVE_VERSION}
fi
case "${ARCH}" in
x86_64) platform=macos-intel ;;
arm64) platform=macos-apple-silicon ;;
*) platform=macos-${ARCH} ;;
esac
mkdir -p "${DEPLOY_DIR}"
cp "${dmg}" "${DEPLOY_DIR}/stopme-${platform}-${postfix}.dmg"
ls -la "${DEPLOY_DIR}"
