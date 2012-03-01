/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/Util/Format.h>
#include <Fabric/Core/Util/Hex.h>
#include <Fabric/Base/Exception.h>

#include <stdint.h>
#include <cmath>

namespace Fabric
{
  namespace Util
  {
    std::string formatHex( size_t count, void const *data )
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
  
  std::string _( char const *data, size_t length, size_t maxLength, char quote )
  {
    std::string result;
    if ( quote )
      result.push_back(quote);
    for ( size_t i=0; i<length; ++i )
    {
      if ( i == maxLength )
      {
        result.append( "..." );
        break;
      }
      else
      {
        char ch = data[i];
        switch ( ch )
        {
          case '\\': result.append( "\\\\" ); break;
          case '\0': result.append( "\\0" ); break;
          case '\n': result.append( "\\n" ); break;
          case '\r': result.append( "\\r" ); break;
          case '\b': result.append( "\\b" ); break;
          case '\t': result.append( "\\t" ); break;
          case '\f': result.append( "\\f" ); break;
          case '\a': result.append( "\\a" ); break;
          case '\v': result.append( "\\v" ); break;
          default:
            if ( quote && ch == quote )
            {
              result.push_back('\\');
              result.push_back(quote);
            }
            else if ( (unsigned char)ch >= (unsigned char)0x80 // utf8
              || isprint((unsigned char)ch) )
              result.push_back(ch);
            else
            {
              result.append( "\\x" );
              result.append( Util::formatHex( 1, &ch ) );
            }
            break;
        }
      }
    }
    if ( quote )
      result.push_back(quote);
    return result;
  }
  
  std::string _( float value )
  {
    static const uint32_t signBit = UINT32_C(0x80000000);
    uint32_t const &valueAsUint32 = *(uint32_t const *)&value;
    if( ( valueAsUint32 & UINT32_C(0x7F800000) ) == UINT32_C(0x7F800000) )
    {
      if( ( valueAsUint32 & UINT32_C(0x007FFFFF) ) == 0 )
        return (valueAsUint32&signBit)? "-Inf": "Inf";
      else
        return "NaN";
    }
    else if( ( valueAsUint32 & ~signBit ) == 0 )
      return (valueAsUint32&signBit)? "-0": "0";

    char buf[20];
    int length = snprintf( buf, 20, "%.7g", value );
    if ( length < 0 || length > 19 )
      throw Exception( "unable to format float32" );
    return buf;
  }
  
  std::string _( double value )
  {
    static const uint64_t signBit = UINT64_C(0x8000000000000000);
    uint64_t const &valueAsUint64 = *(uint64_t const *)&value;
    if( ( valueAsUint64 & UINT64_C(0x7FF0000000000000) ) == UINT64_C(0x7FF0000000000000) )
    {
      if( ( valueAsUint64 & UINT64_C(0x000FFFFFFFFFFFFF) ) == 0 )
        return (valueAsUint64&signBit)? "-Inf": "Inf";
      else
        return "NaN";
    }
    else if( ( valueAsUint64 & ~signBit ) == 0 )
      return (valueAsUint64&signBit)? "-0": "0";

    char buf[40];
    int length = snprintf( buf, 40, "%.16g", value );
    if ( length < 0 || length > 39 )
      throw Exception( "unable to format float64" );
    return buf;
  }
};
