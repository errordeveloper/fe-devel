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
  namespace RT
  {
    class VariableArrayImpl : public ArrayImpl
    {
      friend class Manager;
      friend class Impl;
      friend class SlicedArrayImpl;
      
      struct bits_t
      {
        size_t allocNumMembers;
        size_t numMembers;
        uint8_t *memberDatas;
      };
    
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual void setData( void const *src, void *dst ) const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
     
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
      virtual bool isEquivalentTo( RC::ConstHandle< RT::Impl > const &desc ) const;
      virtual bool isExportable() const;

      // ArrayImpl
      
      virtual size_t getNumMembers( void const *data ) const;
      virtual void const *getImmutableMemberData( void const *data, size_t index ) const;
      virtual void *getMutableMemberData( void *data, size_t index ) const;
      
      // VariableArrayImpl
      
      void setNumMembers( void *data, size_t newNumMembers, void const *defaultMemberData = 0 ) const;
      void setMembers( void *data, size_t numMembers, void const *members ) const;
      void setMembers( void *data, size_t dstOffset, size_t numMembers, void const *members ) const;
      
      void push( void *dst, void const *src ) const;
      void pop( void *dst, void *result ) const;
      void append( void *dst, void const *src ) const;
      
    protected:
    
      VariableArrayImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &memberImpl );
      
      static size_t ComputeAllocatedSize( size_t prevNbAllocated, size_t nbRequested );
            
      void const *getImmutableMemberData_NoCheck( void const *data, size_t index ) const
      { 
        bits_t const *bits = reinterpret_cast<bits_t const *>(data);
        return bits->memberDatas + m_memberSize * index;
      }
      
      void *getMutableMemberData_NoCheck( void *data, size_t index ) const
      { 
        bits_t *bits = reinterpret_cast<bits_t *>(data);
        return bits->memberDatas + m_memberSize * index;
      }    

      void copyMemberDatas( bits_t *dstBits, size_t dstOffset, bits_t const *srcBits, size_t srcOffset, size_t count, bool disposeFirst ) const
      {
        if ( count > 0 )
        {
          FABRIC_ASSERT( srcBits );
          FABRIC_ASSERT( dstBits );
          FABRIC_ASSERT( srcOffset + count <= srcBits->numMembers );
          FABRIC_ASSERT( dstOffset + count <= dstBits->numMembers );
          
          size_t byteSize = m_memberSize * count;
          
          uint8_t *dstMemberDataStart = dstBits->memberDatas + m_memberSize * dstOffset;
          if ( disposeFirst )
            m_memberImpl->disposeDatas( dstMemberDataStart, count, m_memberSize );
          
          size_t srcByteOffset = m_memberSize * srcOffset;
          uint8_t const *srcMemberData = srcBits->memberDatas + srcByteOffset;
          if ( !isMemberShallow() )
          {
            uint8_t *dstMemberDataEnd = dstMemberDataStart + byteSize;
            for ( uint8_t *dstMemberData = dstMemberDataStart; dstMemberData != dstMemberDataEnd; srcMemberData += m_memberSize, dstMemberData += m_memberSize )
              m_memberImpl->setData( srcMemberData, dstMemberData );
          }
          else memcpy( dstMemberDataStart, srcMemberData, byteSize );
        }
      }

    private:

      RC::ConstHandle<Impl> m_memberImpl;
      size_t m_memberSize;
   };
  };
};

#endif //_FABRIC_RT_VARIABLE_ARRAY_IMPL_H
