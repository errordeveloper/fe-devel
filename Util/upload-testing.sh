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


PUB_DIR="$DIST_DIR/pub"
SG_DIR="$DIST_DIR/sg"
if [ "$FORCE" = "1" ]; then
  GIT_OPTS="--git-dir=$PUB_DIR/.git --work-tree=$PUB_DIR"
  #GIT_OPTS="--git-dir=$PUB_DIR/.git"
  rexec git $GIT_OPTS fetch || error
  rexec git $GIT_OPTS merge origin/ver-$VERSION || error
  rexec svn up "$PUB_DIR/Web" || error
else
  rexec mkdir $DIST_DIR || error "$DIST_DIR already exists -- use -f parameter to force update"
  rexec git clone -b ver-$VERSION git://github.com/fabric-engine/PublicDev.git "$PUB_DIR" || error
  rexec ln -s "$PUB_DIR/Web" "$SG_DIR"
  rexec svn co --force http://svn.fabric-engine.com/JSSceneGraphResources/tags/$VERSION "$PUB_DIR/Web" || error
fi

BIN_SRC_DIR="/home/build"
BIN_DIR="$DIST_DIR/bin"
rexec mkdir -p "$BIN_DIR" || error
LOCAL_DIST_DIR="$FABRIC_CORE_PATH/Native/dist"
BINS=
for PLATFORM in Windows-x86_64 Windows-x86 Darwin-universal Linux-i686 Linux-x86_64; do
  if [ "$PLATFORM" = "Windows-x86" -o "$PLATFORM" = "Windows-x86_64" ]; then
    ARCH_EXT=zip
  else
    ARCH_EXT=tar.bz2
  fi
  if [ "$PLATFORM" != "Windows-x86_64" ]; then
    BINS="$BINS $BIN_SRC_DIR/FabricEngine-ChromeExtension-$PLATFORM-$VERSION.crx"
    BINS="$BINS $BIN_SRC_DIR/FabricEngine-NodeModule-$PLATFORM-$VERSION.$ARCH_EXT"
    BINS="$BINS $BIN_SRC_DIR/crx-update-$PLATFORM.xml"
  fi
  BINS="$BINS $BIN_SRC_DIR/FabricEngine-FirefoxExtension-$PLATFORM-$VERSION.xpi"
  if [ "$PLATFORM" = "Windows-x86" ]; then
    BINS="$BINS $BIN_SRC_DIR/FabricEngine-KinectExt-$PLATFORM-$VERSION.$ARCH_EXT"
  fi
  BINS="$BINS $BIN_SRC_DIR/FabricEngine-PythonModule-$PLATFORM-$VERSION.$ARCH_EXT"
  BINS="$BINS $BIN_SRC_DIR/FabricEngine-FileSystemExt-$PLATFORM-$VERSION.$ARCH_EXT"
  BINS="$BINS $BIN_SRC_DIR/FabricEngine-KLTool-$PLATFORM-$VERSION.$ARCH_EXT"
  BINS="$BINS $BIN_SRC_DIR/xpi-update-$PLATFORM.rdf"
done
rexec cp $BINS "$BIN_DIR"/

rexec ln -snf "$VERSION" "/fabric-distribution/testing" || error
rexec ln -snf "$DIST_DIR/bin" "/var/www/dist.testing.fabric-engine.com/$VERSION" || error
rexec ln -snf "$DIST_DIR/bin" "/var/www/documentation.testing.fabric-engine.com/$VERSION" || error
echo "The upload was SUCCESSFUL."
