#!/bin/sh

error()
{
  if [ -n "$1" ]; then
    echo "$@"
  fi
  echo "An error has occurred; aborting.  The upload FAILED."
  exit 1
}

run()
{
  echo "$@"
  "$@"
}

rexec()
{
  run ssh fabric-engine.com "$@"
}

[ -d "$FABRIC_CORE_PATH" ] || error "The FABRIC_CORE_PATH environment variable must be set to the path to the Fabric NativeCore checkout"

FORCE=0
if [ "$1" = "-f" ]; then
  FORCE=1
  shift
fi

VERSION="$1"
[ -n "$VERSION" ] || error "Usage: $0 [-f] <version>"
(echo "$VERSION" | egrep -q '^[0-9]{1,2}\.[0-9]{1,2}\.[0-9]{1,2}-[a-z]+$') || error "Version number must be of the form 1.0.19-beta, 2.1.0-release, etc."
DIST_DIR="/fabric-distribution/$VERSION"


SG_DIR="$DIST_DIR/sg"
if [ "$FORCE" = "1" ]; then
  rexec git --git-dir="$SG_DIR"/.git pull || error
  rexec git --git-dir="$SG_DIR"/.git reset --hard || error
  rexec git --git-dir="$SG_DIR"/.git clean --force || error
  rexec svn up "$SG_DIR" || error
else
  rexec mkdir $DIST_DIR || error "$DIST_DIR already exists -- use -f parameter to force update"
  rexec git clone -b ver-$VERSION git://github.com/fabric-engine/JSSceneGraph.git "$SG_DIR" || error
  rexec svn co --force http://svn.fabric-engine.com/JSSceneGraphResources/tags/$VERSION "$SG_DIR" || error
fi

BIN_SRC_DIR="/home/build"
BIN_DIR="$DIST_DIR/bin"
rexec mkdir -p "$BIN_DIR" || error
LOCAL_DIST_DIR="$FABRIC_CORE_PATH/Native/dist"
for PLATFORM in Windows-x86 Darwin-universal Linux-i686 Linux-x86_64; do
  if [ "$PLATFORM" = "Windows-x86" ]; then
    ARCH_EXT=zip
  else
    ARCH_EXT=tar.bz2
  fi
  rexec cp "$BIN_SRC_DIR/FabricEngine-ChromeExtension-$PLATFORM-$VERSION.crx" "$BIN_DIR/" || error
  rexec cp "$BIN_SRC_DIR/FabricEngine-FirefoxExtension-$PLATFORM-$VERSION.xpi" "$BIN_DIR/" || error
  if [ "$PLATFORM" != "Windows-x86" ]; then
    rexec cp "$BIN_SRC_DIR/FabricEngine-NodeModule-$PLATFORM-$VERSION.tar.bz2" "$BIN_DIR/" || error
  else
    rexec cp "$BIN_SRC_DIR/FabricEngine-KinectExt-$PLATFORM-$VERSION.$ARCH_EXT" "$BIN_DIR/" || error
  fi
  rexec cp "$BIN_SRC_DIR/FabricEngine-PythonModule-$PLATFORM-$VERSION.tar.bz2" "$BIN_DIR/" || error
  rexec cp "$BIN_SRC_DIR/FabricEngine-FileSystemExt-$PLATFORM-$VERSION.$ARCH_EXT" "$BIN_DIR/" || error
  rexec cp "$BIN_SRC_DIR/FabricEngine-KLTool-$PLATFORM-$VERSION.$ARCH_EXT" "$BIN_DIR/" || error
  rexec cp "$BIN_SRC_DIR/crx-update-$PLATFORM.xml" "$BIN_DIR/" || error
  rexec cp "$BIN_SRC_DIR/xpi-update-$PLATFORM.rdf" "$BIN_DIR/" || error
done

rexec ln -snf "$VERSION" "/fabric-distribution/testing" || error
rexec ln -snf "$DIST_DIR/bin" "/var/www/dist.testing.fabric-engine.com/$VERSION" || error
rexec ln -snf "$DIST_DIR/bin" "/var/www/documentation.testing.fabric-engine.com/$VERSION" || error
echo "The upload was SUCCESSFUL."
