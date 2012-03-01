/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_HEX_H
#define _FABRIC_UTIL_HEX_H

#include <string.h>

namespace Fabric
{
  namespace Util
  {
    inline std::string hexBuf( size_t count, void const *data )
    {
      std::string result;
      result.resize( 2 * count );
      for ( size_t i=0; i<count; ++i )
      {
        static char const *hexDigits = "0123456789ABCDEF";
        uint8_t const &byte = ((uint8_t const *)data)[i];
        result[2*i] = hexDigits[byte >> 4];
        result[2*i+1] = hexDigits[byte&0x0F];
      }
      return result;
    }
  };
};

#endif //_FABRIC_UTIL_HEX_H
