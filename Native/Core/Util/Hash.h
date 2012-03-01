/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_HASH_H
#define _FABRIC_UTIL_HASH_H

#include <Fabric/Base/Config.h>

// [pzion 20110215] We really only need the hash functors but
// there doesn't seem to be a standard header to get them from
#if defined(FABRIC_OS_MACOSX) || defined(FABRIC_OS_LINUX)
# include <tr1/functional_hash.h>
#elif defined(FABRIC_OS_WINDOWS)
# include <functional>
#else
# error "Unsupported platform"
#endif

namespace Fabric
{
  namespace Util
  {
    template< class T > struct Hash
      : public std::tr1::hash<T>
    {
    };
  };
};

#endif //_FABRIC_UTIL_HASH_H
