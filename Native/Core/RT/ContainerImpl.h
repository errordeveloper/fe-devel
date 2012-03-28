/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_CONTAINER_IMPL_H
#define _FABRIC_RT_CONTAINER_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace DG
  {
    class Container;
  }

  namespace RT
  {
    class ContainerImpl : public Impl
    {
      friend class Impl;
      friend class Manager;

    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;

      // ContainerImpl
      
      void setValue( RC::Handle<DG::Container> const &container, void *data ) const;
      RC::Handle<DG::Container> getValue( void const *data ) const;

      static void SetData( void const *value, void *data );
      static void DisposeData( void *data );
      static size_t size( void const *data );
      static void resize( void *data, size_t count );
      static std::string GetName( void const *data );
      static bool IsValid( void const *data );

    protected:
      
      ContainerImpl( std::string const &codeName );

      virtual void initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const;

    private:

      static std::string s_undefinedName;
    };
  }
}

#endif //_FABRIC_RT_CONTAINER_IMPL_H
