#!/bin/sh

error()
{
  if [ -n "$1" ]; then
    echo "$@"
  fi
  echo "An error has occurred; aborting."
  exit 1
}

rexec()
{
  echo ssh fabric-engine.com "$@"
  ssh fabric-engine.com "$@"
}

FORCE=0
if [ "$1" = "-f" ]; then
  FORCE=1
  shift
fi

VERSION="$1"
[ -n "$VERSION" ] || error "Usage: $0 [-f] <version>"
(echo "$VERSION" | egrep -q '^[0-9]{1,2}\.[0-9]{1,2}\.[0-9]{1,2}-[a-z]+$') || error "Version number must be of the form 1.0.19-beta, 2.1.0-release, etc."
DIST_DIR="/fabric-distribution/$VERSION"

if [ "$FORCE" -eq 1 ]; then
  LN_FLAGS=-snf
else
  LN_FLAGS=-sn
fi

rexec ln "$LN_FLAGS" "$VERSION" "/fabric-distribution/stable" || error "If you are sure you want to replace the existing stable version, use the -f flag"
rexec ln "$LN_FLAGS" "$DIST_DIR/bin" "/var/www/dist.stable.fabric-engine.com/$VERSION" || error
echo "Version $VERSION is now the STABLE version"
