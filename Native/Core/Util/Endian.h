/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_ENDIAN_H
#define _FABRIC_UTIL_ENDIAN_H

namespace Fabric
{
  namespace Util
  {
    inline bool isLittleEndian()
    {
      static const uint32_t dword = 1;
      return ( *(uint8_t const *)&dword ) != 0;
    }

    inline bool isBigEndian()
    {
      return !isLittleEndian();
    }

    inline uint16_t reverseBytes( uint16_t value )
    {
      uint8_t loByte = (uint8_t)value;
      uint8_t hiByte = (uint8_t)(value >>8 );
      return (uint16_t(loByte) << 8) | uint16_t(hiByte);
    }
    
    inline uint32_t reverseBytes( uint32_t value )
    {
      uint16_t loWord = (uint16_t)value;
      uint16_t hiWord = (uint16_t)(value >> 16 );
      return (uint32_t(reverseBytes(loWord)) << 16) | uint32_t(reverseBytes(hiWord));
    }
    
    inline uint64_t reverseBytes( uint64_t value )
    {
      uint32_t loDWord = (uint32_t)value;
      uint32_t hiDWord = (uint32_t)(value >> 32 );
      return (uint64_t(reverseBytes(loDWord)) << 32) | uint64_t(reverseBytes(hiDWord));
    }
    
    inline void reverseMemcpy( void *dst, void const *src, uint64_t count )
    {
      void const *srcEnd = (uint8_t const *)src + count;
      while ( count > 0 )
      {
        if ( sizeof(long) >= sizeof(uint64_t) )
        {
          if ( count >= sizeof(uint64_t) )
          {
            srcEnd = (uint64_t const *)srcEnd - 1;
            *(uint64_t *)dst = reverseBytes( *(uint64_t const *)srcEnd );
            dst = (uint64_t *)dst + 1;
            count -= sizeof(uint64_t);
            continue;
          }
        }
        
        if ( sizeof(long) >= sizeof(uint32_t) )
        {
          if ( count >= sizeof(uint32_t) )
          {
            srcEnd = (uint32_t const *)srcEnd - 1;
            *(uint32_t *)dst = reverseBytes( *(uint32_t const *)srcEnd );
            dst = (uint32_t *)dst + 1;
            count -= sizeof(uint32_t);
            continue;
          }
        }
        
        if ( sizeof(long) >= sizeof(uint16_t) )
        {
          if ( count >= sizeof(uint16_t) )
          {
            srcEnd = (uint16_t const *)srcEnd - 1;
            *(uint16_t *)dst = reverseBytes( *(uint16_t const *)srcEnd );
            dst = (uint16_t *)dst + 1;
            count -= sizeof(uint16_t);
            continue;
          }
        }
        
        srcEnd = (uint8_t const *)srcEnd - 1;
        *(uint8_t *)dst = *(uint8_t const *)srcEnd;
        dst = (uint8_t *)dst + 1;
        count -= sizeof(uint8_t);
      }
    }
  };
};

#endif //_FABRIC_UTIL_ENDIAN_H
