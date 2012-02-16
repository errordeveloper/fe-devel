/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
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
    
      // Impl
    
      virtual void setData( void const *value, void *data ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
      
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;
      virtual bool isExportable() const;
    
    protected:

      OpaqueImpl( std::string const &codeName, size_t size );
      ~OpaqueImpl();
      
    private:
    
      void *m_defaultData;
    };
  };
};

#endif //_FABRIC_RT_OPAQUE_IMPL_H
