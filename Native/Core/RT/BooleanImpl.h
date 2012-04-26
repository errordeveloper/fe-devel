/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_BOOLEAN_IMPL_H
#define _FABRIC_RT_BOOLEAN_IMPL_H

#include <Fabric/Core/RT/SimpleImpl.h>

namespace Fabric
{
  namespace RT
  {
    class BooleanImpl : public SimpleImpl
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      // Impl
    
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
    
      // ComparableImpl
    
      virtual size_t hash( void const *data ) const;
      virtual int compare( void const *lhsData, void const *rhsData ) const;

      // BooleanImpl

      bool getValue( void const *data ) const
      {
        return *static_cast<bool const *>(data);
      }
      
      void setValue( bool value, void *data ) const
      {
        *static_cast<bool *>(data) = value;
      }
      
    protected:

      BooleanImpl( std::string const &codeName );

      virtual void initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
    };
  }
}

#endif //_FABRIC_RT_BOOLEAN_IMPL_H
