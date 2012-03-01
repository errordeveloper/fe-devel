/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_BASE64_H
#define _FABRIC_UTIL_BASE64_H

#include <string>
#include <stdint.h>

namespace Fabric
{
  namespace Util
  {
    std::string encodeBase64( void const *data, uint64_t count );

    unsigned decodeBase64Size( const char *src );
    unsigned decodeBase64( void *dst, const char *src );
  };
};

#endif //_FABRIC_UTIL_BASE64_H
