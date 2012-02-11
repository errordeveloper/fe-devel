#!/bin/sh

windows_to_unix_path()
{
	UNIX_PATH_RESULT=/$(echo $1 | sed -e 's/\\/\//g' | sed 's#\:##g')
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
		FABRIC_CORE_PATH_WINDOWS_BACKUP=$FABRIC_CORE_PATH
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

  if [ "$FABRIC_BUILD_TYPE" != "Debug" -a "$FABRIC_BUILD_TYPE" != "Release" ]; then
    cat >&2 <<EOF
Usage: souce fabric-build-env.sh [-n] (Debug|Release)
EOF
  else
    FABRIC_BUILD_PATH="$FABRIC_CORE_PATH/Native/build/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH/$FABRIC_BUILD_TYPE"
    FABRIC_DIST_PATH="$FABRIC_CORE_PATH/Native/dist/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH/$FABRIC_BUILD_TYPE"

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
				FABRIC_CORE_PATH=$FABRIC_CORE_PATH_WINDOWS_BACKUP
				windows_to_unix_path $APPDATA
        FABRIC_LIBRARY_DST_DIR="$UNIX_PATH_RESULT/Fabric"
        ;;
    esac
    echo "Creating directory $FABRIC_LIBRARY_DST_DIR"
    mkdir -p "$FABRIC_LIBRARY_DST_DIR"

    FABRIC_EXTS_SRC="$FABRIC_DIST_PATH/Exts"
    mkdir -p "$FABRIC_EXTS_SRC"
    FABRIC_EXTS_DST="$FABRIC_LIBRARY_DST_DIR/Exts"
    echo "Linking $FABRIC_EXTS_DST -> $FABRIC_EXTS_SRC"
    ln -snf "$FABRIC_EXTS_SRC" "$FABRIC_EXTS_DST"

    FABRIC_KL_SRC="$FABRIC_BUILD_PATH/Fabric/Tools/KL/kl"
    FABRIC_KL_DST="$HOME/bin/kl"
    echo "Linking $FABRIC_KL_DST -> $FABRIC_KL_SRC"
    ln -snf "$FABRIC_KL_SRC" "$FABRIC_KL_DST"

    if [ "$FABRIC_BUILD_OS" != "Windows" ]; then
      FABRIC_NODE_MODULE_SRC="$FABRIC_DIST_PATH/NodeModule"
      FABRIC_NODE_MODULE_DIR="$HOME/node_modules"
      mkdir -p "$FABRIC_NODE_MODULE_DIR"
      FABRIC_NODE_MODULE_DST="$FABRIC_NODE_MODULE_DIR/Fabric"
      echo "Linking $FABRIC_NODE_MODULE_DST -> $FABRIC_NODE_MODULE_SRC"
      ln -snf "$FABRIC_NODE_MODULE_SRC" "$FABRIC_NODE_MODULE_DST"

      FABRIC_PYTHON_MODULE_SRC="$FABRIC_DIST_PATH/PythonModule"
      FABRIC_PYTHON_MODULE_DIR="$HOME/python_modules"
      mkdir -p "$FABRIC_PYTHON_MODULE_DIR"
      FABRIC_PYTHON_MODULE_DST="$FABRIC_PYTHON_MODULE_DIR/fabric"
      echo "Linking $FABRIC_PYTHON_MODULE_DST -> $FABRIC_PYTHON_MODULE_SRC"
      ln -snf "$FABRIC_PYTHON_MODULE_SRC" "$FABRIC_PYTHON_MODULE_DST"
    fi

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
					pushd $FABRIC_NPAPI_SRC
					regsvr32 //s npFabricPlugin.dll
					popd
				fi
			else
				echo "Creating directory $FABRIC_NPAPI_DST_DIR"
				mkdir -p "$FABRIC_NPAPI_DST_DIR"
				echo "Linking $FABRIC_NPAPI_DST -> $FABRIC_NPAPI_SRC"
				ln -snf "$FABRIC_NPAPI_SRC" "$FABRIC_NPAPI_DST"
			fi
		else
		  if [ "$FABRIC_BUILD_OS" = "Windows" ]; then
			  if [ -f $FABRIC_NPAPI_SRC/npFabricPlugin.dll ]; then
					echo "Unregristering NPAPI plugin"
					pushd $FABRIC_NPAPI_SRC
					regsvr32 //s //u npFabricPlugin.dll
					popd
				fi
			else
  			echo "Removing $FABRIC_NPAPI_DST"
	  		rm -f "$FABRIC_NPAPI_DST"
			fi
		fi
  fi
fi
