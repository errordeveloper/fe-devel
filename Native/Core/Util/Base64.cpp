/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Base64.h"
#include "Debug.h"
#include <ctype.h>
#include <string>

#if defined(FABRIC_OS_LINUX)
	#include <string.h>
#endif

namespace Fabric
{
  namespace Util
  {
    std::string encodeBase64( void const *_data, uint64_t count )
    {
      static uint8_t const base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      uint8_t const *data = (uint8_t const *)_data;
      std::string result;
      while ( count > 0 )
      {
        char block[] = "====";
        
        block[0] = base64Chars[data[0]>>2];
        
        if ( count > 1 )
          block[1] = base64Chars[((data[0]&0x03u)<<4) | ((data[1]&0xF0u)>>4)];
        else
          block[1] = base64Chars[((data[0]&0x03u)<<4)];
        
        if ( count > 2 )
          block[2] = base64Chars[((data[1]&0x0Fu)<<2) | ((data[2]&0xC0u)>>6)];
        else if ( count > 1 )
          block[2] = base64Chars[((data[1]&0x0Fu)<<2)];
        
        if ( count > 2 )
          block[3] = base64Chars[data[2]&0x3Fu];
          
        result.append( block, 4 );
           
        uint64_t advance = count;
        if ( advance > 3 )
          advance = 3;
        data += advance;
        count -= advance;
      }
      return result;
    }

    unsigned decodeBase64Size( const char *src )
    {
      // [pzion 20100930] This is a gross upper bound for the required size; it
      // may be smaller if there is whitespace
      return (3*strlen(src)+3)/4;
    }
    
    static inline bool nextVal( const char *&src, uint8_t &val )
    {
      static const uint8_t invalid = 255;
      static uint8_t lookupTable[256];
      static bool lookupTableInitialized = false;
      if ( !lookupTableInitialized )
      {
        for ( int i=0; i<256; ++i )
          lookupTable[i] = invalid;
          
        static uint8_t const base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        uint8_t const *p = base64Chars;
        while ( *p )
        {
          lookupTable[*p] = p - base64Chars;
          ++p;
        }
        
        lookupTableInitialized = true;
      }
      
      while ( *src )
      {
        val = lookupTable[size_t(*src++)];
        if ( val != invalid )
          return true;
      }
      return false;
    }
    
    unsigned decodeBase64( void *_dst, const char *src )
    {
      uint8_t *dst = static_cast<uint8_t *>(_dst);
      unsigned size = 0;
      while ( *src )
      {
        uint8_t a;
        if ( nextVal( src, a ) )
        {
          uint8_t b;
          if ( nextVal( src, b ) )
          {
            dst[size++] = (a << 2) | (b >> 4);
            
            uint8_t c;
            if ( nextVal( src, c ) )
            {
              dst[size++] = (b << 4) | (c >> 2);
              
              uint8_t d;
              if ( nextVal( src, d ) )
                dst[size++] = (c << 6) | (d >> 0);
            }
          }
        }
      }
      return size;
    }
  };
};
