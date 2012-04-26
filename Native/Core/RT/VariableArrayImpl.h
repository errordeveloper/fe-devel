/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_VARIABLE_ARRAY_IMPL_H
#define _FABRIC_RT_VARIABLE_ARRAY_IMPL_H

#include <Fabric/Core/RT/ArrayImpl.h>
#include <Fabric/Base/Util/Bits.h>
#include <Fabric/Base/Util/AtomicSize.h>

#include <string.h>
#include <algorithm>

namespace Fabric
{
  namespace CG
  {
    class VariableArrayAdapter;
  }
  
  namespace RT
  {
    class VariableArrayImpl : public ArrayImpl
    {
      friend class Manager;
      friend class Impl;
      friend class SlicedArrayImpl;
      friend class CG::VariableArrayAdapter;
      
      struct bits_t
      {
        Util::AtomicSize refCount;
        size_t allocNumMembers;
        size_t numMembers;
        uint8_t memberDatas[0];
      };
    
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
     
      virtual bool isEquivalentTo( RC::ConstHandle< RT::Impl > const &desc ) const;

      // ArrayImpl
      
      virtual size_t getNumMembers( void const *data ) const;
      virtual void const *getImmutableMemberData( void const *data, size_t index ) const;
      virtual void *getMutableMemberData( void *data, size_t index ) const;
      
      // VariableArrayImpl
      
      void setNumMembers( void *data, size_t newNumMembers, void const *defaultMemberData = 0, size_t defaultMemberStride = 0 ) const;
      void setMembers( void *data, size_t numMembers, void const *members ) const;
      void setMembers( void *data, size_t dstOffset, size_t numMembers, void const *members ) const;
      
      void push( void *dst, void const *src ) const;
      void pop( void *dst, void *result ) const;
      void append( void *dst, void const *src ) const;
      
    protected:
    
      VariableArrayImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &memberImpl );

      virtual void initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const;
      
      static size_t ComputeAllocatedSize( size_t prevNbAllocated, size_t nbRequested );
            
      void const *getImmutableMemberData_NoCheck( void const *data, size_t index ) const
      { 
        bits_t const *bits = *reinterpret_cast<bits_t const * const *>(data);
        return bits->memberDatas + m_memberSize * index;
      }
      
      void *getMutableMemberData_NoCheck( void *data, size_t index ) const
      { 
        bits_t *bits = prepareForModify( data );
        return bits->memberDatas + m_memberSize * index;
      }    

      bits_t *prepareForModify( void *data ) const
      {
        bits_t *bits = *static_cast<bits_t **>( data );
        if ( bits->refCount.getValue() > 1 )
          bits = duplicate( data );
        FABRIC_ASSERT( bits->refCount.getValue() == 1 );
        return bits;
      }

      bits_t *duplicate( void *data ) const;

    private:

      RC::ConstHandle<Impl> m_memberImpl;
      size_t m_memberSize;
    };
  }
}

#endif //_FABRIC_RT_VARIABLE_ARRAY_IMPL_H
