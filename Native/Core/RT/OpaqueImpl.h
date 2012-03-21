/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_OPAQUE_IMPL_H
#define _FABRIC_RT_OPAQUE_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace RT
  {
    class OpaqueImpl : public Impl
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      // Impl
    
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
      
      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;
    
    protected:

      OpaqueImpl( std::string const &codeName, size_t size );
      ~OpaqueImpl();

      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const;
      
    private:
    
      void *m_defaultData;
    };
  }
}

#endif //_FABRIC_RT_OPAQUE_IMPL_H
