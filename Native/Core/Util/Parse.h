/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_PARSE_H
#define _FABRIC_UTIL_PARSE_H

#include <string>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

namespace Fabric
{
  namespace Util
  {
    uint64_t parseUInt64( char const *data, size_t length );
    inline uint64_t parseUInt64( char const *cString )
    {
      return parseUInt64( cString, strlen(cString) );
    }
    inline uint64_t parseUInt64( std::string const &string )
    {
      return parseUInt64( string.data(), string.length() );
    }
    
    double parseDouble( char const *data, size_t length );
    inline double parseDouble( char const *cString )
    {
      return atof( cString );
    }
    inline double parseDouble( std::string const &string )
    {
      return atof( string.c_str() );
    }

    std::string parseQuotedString( char const *data, size_t length );
    inline std::string parseQuotedString( char const *cString )
    {
      return parseQuotedString( cString, strlen(cString) );
    }
    inline std::string parseQuotedString( std::string const &string )
    {
      return parseQuotedString( string.data(), string.length() );
    }
  }
}

#endif //_FABRIC_UTIL_PARSE_H
