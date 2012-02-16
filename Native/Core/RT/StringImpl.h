/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_RT_STRING_IMPL_H
#define _FABRIC_RT_STRING_IMPL_H

#include <Fabric/Core/RT/ComparableImpl.h>
#include <Fabric/Base/Util/AtomicSize.h>
#include <Fabric/Base/Util/Bits.h>

#include <string.h>
#include <stdlib.h>

namespace Fabric
{
  namespace RT
  {
    class StringImpl : public ComparableImpl
    {
      friend class Manager;
      friend class DictImpl;
      
      struct bits_t
      {
        Util::AtomicSize refCount;
        size_t allocSize;
        size_t length;
        char cStr[0]; // [pzion 20101225] Will be extended to length of string plus null terminator
      };

    public:
    
      // Impl
      
      virtual void setData( void const *value, void *data ) const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
      virtual bool isExportable() const;
    
      // ComparableImpl
    
      virtual size_t hash( void const *data ) const;
      virtual int compare( void const *lhsData, void const *rhsData ) const;
      
      // StringImpl
      
      char const *getValueData( void const *src ) const
      {
        FABRIC_ASSERT( src );
        bits_t const * const *bitsPtr = static_cast<bits_t const * const *>( src );
        bits_t const *bits = *bitsPtr;
        char const *result;
        if ( bits )
          result = bits->cStr;
        else result = 0;
        return result;
      }
      
      size_t getValueLength( void const *src ) const
      {
        FABRIC_ASSERT( src );
        bits_t const *bits = *static_cast<bits_t const * const *>( src );
        size_t result;
        if ( bits )
          result = bits->length;
        else result = 0;
        return result;
      }

      static void SetValue( char const *data, size_t length, void *dst )
      {
        FABRIC_ASSERT( dst );
        Prepare( length, length + 1, false, dst );
        Replace( data, length, dst );
      }

      static void Append( void *dst, void const *src )
      {
        FABRIC_ASSERT( dst );
        FABRIC_ASSERT( src );
        
        bits_t *srcBits = *reinterpret_cast<bits_t * const *>(src);
        if ( srcBits )
        {
          bits_t *&dstBits = *static_cast<bits_t **>( dst );
          if ( dstBits )
          {
            size_t totalLength = dstBits->length + srcBits->length;
            size_t allocSize = std::max( Util::nextPowerOfTwoMinusOne( totalLength + 1 ), size_t(32) );
            Prepare( totalLength, allocSize, true, dst );
            memcpy( &dstBits->cStr[dstBits->length], srcBits->cStr, srcBits->length + 1 );
            dstBits->length = totalLength;
          }
          else
          {
            dstBits = srcBits;
            dstBits->refCount.increment();
          }
        }
      }

    protected:
      
      StringImpl( std::string const &codeName );
      
      static size_t AllocSizeForLength( size_t length )
      {
        size_t result;
        if ( length < 32 )
          result = 32;
        else if ( length < 32 )
          result = 32;
        else if ( length < 64 )
          result = 64;
        else if ( length < 128 )
          result = 128;
        else if ( length < 256 )
          result = 256;
        else result = length + 1;
        return result;
      }
    
      static void Prepare( size_t length, size_t allocSize, bool retain, void *dst )
      {
        FABRIC_ASSERT( dst );
        bits_t *&bits = *static_cast<bits_t **>(dst);
        if ( length )
        {
          bits_t *retainedBits = 0;
          
          if ( bits && bits->refCount.getValue() > 1 )
          {
            if ( retain )
              retainedBits = bits;
            else if ( bits->refCount.decrementAndGetValue() == 0 )
              free( bits );
            bits = 0;
          }
          FABRIC_ASSERT( !bits || bits->refCount.getValue() == 1 );
          
          if ( !bits )
          {
            bits = (bits_t *)malloc( sizeof( bits_t ) + allocSize );
            bits->refCount.setValue( 1 );
            bits->allocSize = allocSize;
            if ( retainedBits )
            {
              FABRIC_ASSERT( retainedBits->length + 1 < allocSize );
              memcpy( bits->cStr, retainedBits->cStr, retainedBits->length + 1 );
              bits->length = retainedBits->length;
              if ( retainedBits->refCount.decrementAndGetValue() == 0 )
                free( retainedBits );
            }
          }
          else if ( length + 1 > bits->allocSize )
          {
            bits = (bits_t *)realloc( bits, sizeof( bits_t ) + allocSize );
            bits->allocSize = allocSize;
          }
        }
        else if ( bits )
        {
          if ( bits->refCount.decrementAndGetValue() == 0 )
            free( bits );
          bits = 0;
        }
      }

      static void Replace( char const *cStr, size_t length, void *dst )
      {
        FABRIC_ASSERT( dst );
        bits_t *bits = *static_cast<bits_t **>(dst);
        if ( length )
        {
          FABRIC_ASSERT( cStr );
          FABRIC_ASSERT( bits );
          FABRIC_ASSERT( bits->refCount.getValue() == 1 );
          FABRIC_ASSERT( length + 1 <= bits->allocSize );
          memcpy( bits->cStr, cStr, length );
          bits->cStr[length] = '\0';
          bits->length = length;
        }
        else FABRIC_ASSERT( !bits );
      }

      static char *GetMutableValueData( void *data, size_t length )
      {
        FABRIC_ASSERT( data );
        
        Prepare( length, length + 1, false, data );
        
        bits_t * const *bitsPtr = static_cast<bits_t * const *>( data );
        bits_t *bits = *bitsPtr;
        char *result;
        if ( bits )
        {
          bits->length = length;
          result = bits->cStr;
          result[length] = '\0';
        }
        else result = 0;
        return result;
      }
    };
  };
};

#endif //_FABRIC_RT_STRING_IMPL_H
