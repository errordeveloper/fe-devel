#!/bin/sh

windows_to_unix_path()
{
  if [[ $1 == *\\* ]]
  then
    UNIX_PATH_RESULT=/$(echo $1 | sed 's/\\/\//g' | sed 's/\://g')
  else
    UNIX_PATH_RESULT=$1
  fi
}

remove_link()
{
  echo "Removing link $1"
  if [ "$FABRIC_BUILD_OS" = "Windows" ]; then
    if [ -d $1 ]; then
      rmdir "$1"
    fi
  else
    rm -f "$1"
  fi
}

FABRIC_BUILD_OS=$(uname -s)
if [[ "$FABRIC_BUILD_OS" == *W32* ]]
then
  FABRIC_BUILD_OS="Windows"
  FABRIC_BUILD_ARCH="x86"
	if [ -z "$FABRIC_CORE_PATH" ]; then
		cat >&2 <<EOF
The FABRIC_CORE_PATH environment variable must be set
to the path to the Fabric NativeCore checkout
EOF
	fi
  windows_to_unix_path $FABRIC_CORE_PATH
  FABRIC_CORE_PATH=$UNIX_PATH_RESULT
	FABRIC_DIST_BASE_PATH="$FABRIC_CORE_PATH/dist/Native/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH"
	FABRIC_NPAPI_PATH=
  if [ -f $FABRIC_DIST_BASE_PATH/Debug/NPAPI/npFabricPlugin.dll ]; then
		FABRIC_NPAPI_PATH="$FABRIC_DIST_BASE_PATH/Debug/NPAPI"
	fi
  if [ -f $FABRIC_DIST_BASE_PATH/Release/NPAPI/npFabricPlugin.dll ]; then
		FABRIC_NPAPI_PATH="$FABRIC_DIST_BASE_PATH/Release/NPAPI"
	fi
	if [ -z "$FABRIC_NPAPI_PATH" ]; then
		echo "Warning: Fabric NPAPI plugin can not be unregistered (npFabricPlugin.dll was not found in $FABRIC_DIST_BASE_PATH/Debug/NPAPI or $FABRIC_DIST_BASE_PATH/Release/NPAPI)"
	else
		echo "Unregistering npFabricPlugin.dll"
    regsvr32 //s //u $FABRIC_NPAPI_PATH/npFabricPlugin.dll
	fi
fi

case "$FABRIC_BUILD_OS" in
  Darwin)
    FABRIC_LIBRARY_DST_DIR="$HOME/Library/Fabric"
    ;;
  Linux)
    FABRIC_LIBRARY_DST_DIR="$HOME/.fabric"
    ;;
  Windows)
    windows_to_unix_path $APPDATA
    FABRIC_LIBRARY_DST_DIR="$UNIX_PATH_RESULT/Fabric"
    ;;
esac

if [ -d $FABRIC_LIBRARY_DST_DIR ]; then
  remove_link "$FABRIC_LIBRARY_DST_DIR/Exts"
fi

remove_link "$HOME/python_modules/fabric"

if [ "$FABRIC_BUILD_OS" != "Windows" ]; then
	remove_link "$HOME/bin/kl"
	remove_link "$HOME/node_modules/Fabric"
fi

case "$FABRIC_BUILD_OS" in
  Darwin)
    FABRIC_NPAPI_DST_DIR="$HOME/Library/Internet Plug-Ins"
    FABRIC_NPAPI_DST="$FABRIC_NPAPI_DST_DIR/Fabric.$FABRIC_BUILD_ARCH.plugin"
		remove_link "$FABRIC_NPAPI_DST"
    ;;
  Linux)
    FABRIC_NPAPI_DST_DIR="$HOME/.mozilla/plugins"
    FABRIC_NPAPI_DST="$FABRIC_NPAPI_DST_DIR/libFabricPlugin.so"
		remove_link "$FABRIC_NPAPI_DST"
    ;;
esac
