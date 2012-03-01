/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_BUILD_H
#define _FABRIC_BUILD_H

#include <time.h>

namespace Fabric
{
  extern char const *buildName;
  extern char const *buildFullVersion;
  extern char const *buildPureVersion;
  extern char const *buildDesc;
  extern char const *buildCopyright;
  extern char const *buildURL;
  extern size_t const buildCacheGeneration;
  extern char const *buildOS;
  extern char const *buildArch;
  extern char const *runningArch;
}

#endif //_FABRIC_BUILD_H
