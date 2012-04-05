/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_STRUCT_IMPL_H
#define _FABRIC_RT_STRUCT_IMPL_H

#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/StructMemberInfo.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    class StructImpl : public Impl
    {
      friend class Manager;
    
      typedef std::vector< size_t > MemberOffsetVector;
      typedef Util::UnorderedMap< std::string, size_t > NameToIndexMap;
      
    public:
      REPORT_RC_LEAKS
      
      // Impl
      
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle< RT::Impl > const &desc ) const;
       
      // StructImpl
          
      size_t getNumMembers() const
      {
        return m_numMembers;
      }

      void setDefaultValues( StructMemberInfoVector const &memberInfos ) const;
      
      StructMemberInfo const &getMemberInfo( size_t index ) const
      {
        if ( index < 0 || index >= m_numMembers )
          throw Exception( "index out of range" );
        return m_memberInfos[index];
      }
      
      void const *getImmutableMemberData( void const *data, size_t index ) const
      {
        if ( index < 0 || index >= m_numMembers )
          throw Exception( "index out of range" );
        return getImmutableMemberData_NoCheck( data, index );
      }
      
      void *getMutableMemberData( void *data, size_t index ) const
      {
        if ( index < 0 || index >= m_numMembers )
          throw Exception( "index out of range" );
        return getMutableMemberData_NoCheck( data, index );
      }  
     
      bool hasMember( std::string const &name ) const
      {
        return m_nameToIndexMap.find( name ) != m_nameToIndexMap.end();
      }
      
      size_t getMemberIndex( std::string const &name ) const
      {
         NameToIndexMap::const_iterator it = m_nameToIndexMap.find( name );
        if ( it == m_nameToIndexMap.end() )
          throw Exception( "member not found" );
        return it->second;
      }

      StructMemberInfoVector const &getMemberInfos() const
      {
        return m_memberInfos;
      }

    protected:
    
      StructImpl(
        std::string const &codeName,
        StructMemberInfoVector const &memberInfos
        );
      ~StructImpl();

      virtual void initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t impl ) const;
      
      void const *getImmutableMemberData_NoCheck( void const *data, size_t index ) const
      {
        return static_cast<uint8_t const *>(data) + m_memberOffsets[index];
      }
      
      void *getMutableMemberData_NoCheck( void *data, size_t index ) const
      {
        return static_cast<uint8_t *>(data) + m_memberOffsets[index];
      }  

    private:
    
      std::string m_name;
      mutable StructMemberInfoVector m_memberInfos;
      size_t m_numMembers;
      MemberOffsetVector m_memberOffsets;
      NameToIndexMap m_nameToIndexMap;
      void *m_defaultData;
    };
  }
}

#endif //_FABRIC_RT_STRUCT_IMPL_H
