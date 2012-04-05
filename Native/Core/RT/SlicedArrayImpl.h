/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_SLICED_ARRAY_IMPL_H
#define _FABRIC_RT_SLICED_ARRAY_IMPL_H

#include <Fabric/Core/RT/ArrayImpl.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Base/Util/Bits.h>
#include <Fabric/Base/Util/AtomicSize.h>

#include <string.h>
#include <algorithm>

namespace Fabric
{
  namespace CG
  {
    class SlicedArrayAdapter;
  };
  
  namespace RT
  {
    class VariableArrayImpl;
    
    class SlicedArrayImpl : public ArrayImpl
    {
      friend class Manager;
      friend class Impl;
      friend class CG::SlicedArrayAdapter;
      
      struct bits_t
      {
        size_t offset;
        size_t size;
        uint8_t va;
      };
    
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
      
      virtual bool isEquivalentTo( RC::ConstHandle<RT::Impl> const &desc ) const;

      // ArrayImpl
      
      virtual size_t getNumMembers( void const *data ) const;
      virtual void const *getImmutableMemberData( void const *data, size_t index ) const;
      virtual void *getMutableMemberData( void *data, size_t index ) const;
      
      // SlicedArrayImpl
      
      size_t getOffset( void const *data ) const;
      size_t getSize( void const *data ) const;
      void setNumMembers( void *data, size_t numMembers, void const *defaultMemberData ) const;
      
    protected:
    
      SlicedArrayImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &memberImpl );
      ~SlicedArrayImpl();

      virtual void initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const;

    private:

      RC::ConstHandle<Impl> m_memberImpl;
      size_t m_memberSize;
      RC::ConstHandle<VariableArrayImpl> m_variableArrayImpl;
      void *m_defaultData;
    };
  }
}

#endif //_FABRIC_RT_SLICED_ARRAY_IMPL_H
