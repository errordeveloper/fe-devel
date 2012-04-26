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

unix_to_windows_path()
{
  WINDOWS_PATH_RESULT=$(echo $1 | sed 's/^\/\(.\)\//\1:\\/' | sed 's/\//\\/g')
}

create_link()
{
  echo "Linking $2 -> $1"
  if [ "$FABRIC_BUILD_OS" = "Windows" ]; then
    # Note: can't test a Windows junction with -e, -L, -d, -f; they all report false
    rmdir $2
    unix_to_windows_path $2
    ARG1=$WINDOWS_PATH_RESULT
    unix_to_windows_path $1
    ARG2=$WINDOWS_PATH_RESULT
    cmd //C mklink //J $ARG1 $ARG2
  else
    ln -snf "$1" "$2"
  fi
}

if [ -z "$FABRIC_CORE_PATH" ]; then
  cat >&2 <<EOF
The FABRIC_CORE_PATH environment variable must be set
to the path to the Fabric NativeCore checkout
EOF
else
  FABRIC_NPAPI_LINK=0
  FABRIC_BUILD_OS=$(uname -s)
  if [[ "$FABRIC_BUILD_OS" == *W32* ]]
  then
    FABRIC_BUILD_OS="Windows"
    FABRIC_BUILD_ARCH="x86"
    windows_to_unix_path $FABRIC_CORE_PATH
    FABRIC_CORE_PATH=$UNIX_PATH_RESULT
  else
    if [ "$FABRIC_BUILD_OS" = "Darwin" ]; then
      FABRIC_BUILD_ARCH=universal
    else
      FABRIC_BUILD_ARCH=$(uname -m)
    fi
  fi
  FABRIC_BUILD_TYPE=

  OPTIND=0
  while getopts ":n" opt; do
    case $opt in
      n)
        FABRIC_NPAPI_LINK=1
        ;;
      \?)
        echo "Invalid option: -$OPTARG" >&2
        usage
        ;;
    esac
  done
  shift $(($OPTIND-1))
  FABRIC_BUILD_TYPE=$1

  if [ "$FABRIC_BUILD_OS" == "Windows" ]; then
		if [ "$FABRIC_BUILD_TYPE" == "Debug64" ]; then
			FABRIC_BUILD_TYPE="Debug"
			FABRIC_BUILD_ARCH="x86_64"
			echo "Build architecture set to 64 bits"
		fi
		if [ "$FABRIC_BUILD_TYPE" == "Release64" ]; then
			FABRIC_BUILD_TYPE="Release"
			FABRIC_BUILD_ARCH="x86_64"
			echo "Build architecture set to 64 bits"
		fi
	fi

  if [ "$FABRIC_BUILD_TYPE" != "Debug" -a "$FABRIC_BUILD_TYPE" != "Release" ]; then
    if [ "$FABRIC_BUILD_OS" == "Windows" ]; then
	    cat >&2 <<EOF
Usage: souce fabric-build-env.sh [-n] (Debug|Debug64|Release|Release64)
EOF
		else
	    cat >&2 <<EOF
Usage: souce fabric-build-env.sh [-n] (Debug|Release)
EOF
		fi
  else
    FABRIC_BUILD_PATH="$FABRIC_CORE_PATH/build/Native/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH/$FABRIC_BUILD_TYPE"
    FABRIC_DIST_PATH="$FABRIC_CORE_PATH/dist/Native/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH/$FABRIC_BUILD_TYPE"

    echo "Exporting FABRIC_BUILD_OS=$FABRIC_BUILD_OS"
    export FABRIC_BUILD_OS="$FABRIC_BUILD_OS"
    echo "Exporting FABRIC_BUILD_ARCH=$FABRIC_BUILD_ARCH"
    export FABRIC_BUILD_ARCH="$FABRIC_BUILD_ARCH"
    echo "Exporting FABRIC_BUILD_TYPE=$FABRIC_BUILD_TYPE"
    export FABRIC_BUILD_TYPE="$FABRIC_BUILD_TYPE"

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
    echo "Creating directory $FABRIC_LIBRARY_DST_DIR"
    mkdir -p "$FABRIC_LIBRARY_DST_DIR"

    FABRIC_EXTS_SRC="$FABRIC_DIST_PATH/Exts"
    mkdir -p "$FABRIC_EXTS_SRC"
    FABRIC_EXTS_DST="$FABRIC_LIBRARY_DST_DIR/Exts"
    create_link "$FABRIC_EXTS_SRC" "$FABRIC_EXTS_DST"

    if [ "$FABRIC_BUILD_OS" != "Windows" ]; then
	    FABRIC_KL_SRC="$FABRIC_BUILD_PATH/Fabric/Tools/KL/kl"
		  FABRIC_KL_DST="$HOME/bin/kl"
			create_link "$FABRIC_KL_SRC" "$FABRIC_KL_DST"
		fi

    FABRIC_PYTHON_MODULE_SRC="$FABRIC_DIST_PATH/PythonModule"
    FABRIC_PYTHON_MODULE_DIR="$HOME/python_modules"
    FABRIC_PYTHON_MODULE_DST="$FABRIC_PYTHON_MODULE_DIR/fabric"
    mkdir -p "$FABRIC_PYTHON_MODULE_DIR"
    create_link "$FABRIC_PYTHON_MODULE_SRC" "$FABRIC_PYTHON_MODULE_DST"

    FABRIC_NODE_MODULE_SRC="$FABRIC_DIST_PATH/NodeModule"
    FABRIC_NODE_MODULE_DIR="$HOME/node_modules"
    FABRIC_NODE_MODULE_DST="$FABRIC_NODE_MODULE_DIR/Fabric"
    mkdir -p "$FABRIC_NODE_MODULE_DIR"
    create_link "$FABRIC_NODE_MODULE_SRC" "$FABRIC_NODE_MODULE_DST"

    case "$FABRIC_BUILD_OS" in
      Darwin)
        FABRIC_NPAPI_SRC="$FABRIC_DIST_PATH/NPAPI/Fabric.$FABRIC_BUILD_ARCH.plugin"
        FABRIC_NPAPI_DST_DIR="$HOME/Library/Internet Plug-Ins"
        FABRIC_NPAPI_DST="$FABRIC_NPAPI_DST_DIR/Fabric.$FABRIC_BUILD_ARCH.plugin"
        ;;
      Linux)
        FABRIC_NPAPI_SRC="$FABRIC_DIST_PATH/NPAPI/libFabricPlugin.so"
        FABRIC_NPAPI_DST_DIR="$HOME/.mozilla/plugins"
        FABRIC_NPAPI_DST="$FABRIC_NPAPI_DST_DIR/libFabricPlugin.so"
        ;;
      Windows)
        FABRIC_NPAPI_SRC="$FABRIC_DIST_PATH/NPAPI"
    esac
    if [ "$FABRIC_NPAPI_LINK" -eq 1 ]; then
      if [ "$FABRIC_BUILD_OS" = "Windows" ]; then
        if [ -f $FABRIC_NPAPI_SRC/npFabricPlugin.dll ]; then
          echo "Regristering NPAPI plugin"
          regsvr32 //s $FABRIC_NPAPI_SRC/npFabricPlugin.dll
        fi
      else
        mkdir -p "$FABRIC_NPAPI_DST_DIR"
        create_link "$FABRIC_NPAPI_SRC" "$FABRIC_NPAPI_DST"
      fi
    else
      if [ "$FABRIC_BUILD_OS" = "Windows" ]; then
        if [ -f $FABRIC_NPAPI_SRC/npFabricPlugin.dll ]; then
          echo "Unregristering NPAPI plugin"
          regsvr32 //s //u $FABRIC_NPAPI_SRC/npFabricPlugin.dll
        fi
      else
        echo "Removing $FABRIC_NPAPI_DST"
        rm -f "$FABRIC_NPAPI_DST"
      fi
    fi
  fi
fi
