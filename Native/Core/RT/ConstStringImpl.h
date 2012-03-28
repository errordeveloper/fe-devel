/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_CONST_STRING_IMPL_H
#define _FABRIC_RT_CONST_STRING_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace RT
  {
    class ConstStringImpl : public Impl
    {
      friend class Manager;
      friend class Impl;
      
      struct bits_t
      {
        char const *data;
        size_t length;
      };
      
    public:
      REPORT_RC_LEAKS
          
      // Impl
    
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle<RT::Impl> const &impl ) const;
      
      // ConstStringImpl

      char const *getValueData( void const *src ) const
      {
        bits_t const *bits = static_cast<bits_t const *>( src );
        return bits->data;
      }
      
      size_t getValueLength( void const *src ) const
      {
        bits_t const *bits = static_cast<bits_t const *>( src );
        return bits->length;
      }
      
      std::string toString( void const *data ) const;
            
    protected:
    
      ConstStringImpl( std::string const &codeName );

      virtual void initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const;
    };
  }
}

#endif //_FABRIC_RT_CONST_STRING_IMPL_H
