/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/RT/StructImpl.h>

#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {
    StructImpl::StructImpl( std::string const &codeName, StructMemberInfoVector const &memberInfos )
      : Impl( codeName, DT_STRUCT )
      , m_memberInfos( memberInfos )
      , m_numMembers( memberInfos.size() )
      , m_defaultData( 0 )
    {
      m_isShallow = true;
      m_isNoAliasSafe = true;
      m_isExportable = true;
      m_memberOffsets.push_back( 0 );
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = getMemberInfo(i);
        m_memberOffsets.push_back( m_memberOffsets.back() + memberInfo.desc->getAllocSize() );
        m_nameToIndexMap.insert( NameToIndexMap::value_type( memberInfo.name, i ) );
        m_isShallow = m_isShallow && memberInfo.desc->isShallow();
        m_isNoAliasSafe = m_isNoAliasSafe && memberInfo.desc->isNoAliasSafe();
        m_isExportable = m_isExportable && memberInfo.desc->isExportable();
      }
      
      size_t size = m_memberOffsets[m_numMembers];
      setSize( size );
      m_defaultData = malloc( size );
      memset( m_defaultData, 0, size );
      setDefaultValues( memberInfos );
    }
    
    void StructImpl::setDefaultValues( StructMemberInfoVector const &memberInfos ) const
    {
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = memberInfos[i];
        void const *srcMemberData;
        if ( !memberInfo.defaultData.size() )
          srcMemberData = memberInfo.desc->getDefaultData();
        else srcMemberData = &memberInfo.defaultData[0];
        void *dstMemberData = static_cast<uint8_t *>(m_defaultData) + m_memberOffsets[i];
        memberInfo.desc->setData( srcMemberData, dstMemberData );
      }
    }
    
    StructImpl::~StructImpl()
    {
      disposeData( m_defaultData );
      free( m_defaultData );
    }
    
    void const *StructImpl::getDefaultData() const
    {
      return m_defaultData;
    }
    
    void StructImpl::setData( void const *srcData, void *dstData ) const
    {
      if ( m_isShallow )
        memcpy( dstData, srcData, getAllocSize() );
      else
      {
        for ( size_t i=0; i<m_numMembers; ++i )
        {
          StructMemberInfo const &memberInfo = m_memberInfos[i];
          size_t memberOffset = m_memberOffsets[i];
          void const *srcMemberData = static_cast<uint8_t const *>(srcData) + memberOffset;
          void *dstMemberData = static_cast<uint8_t *>(dstData) + memberOffset;
          memberInfo.desc->setData( srcMemberData, dstMemberData );
        }
      }
    }
    
    void StructImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = m_memberInfos[i];
        void const *memberData = static_cast<uint8_t const *>(data) + m_memberOffsets[i];
        JSON::Encoder memberEncoder = objectEncoder.makeMember( memberInfo.name );
        memberInfo.desc->encodeJSON( memberData, memberEncoder );
      }
    }
    
    void StructImpl::decodeJSON( JSON::Entity const &entity, void *data ) const
    {
      entity.requireObject();
        
      size_t membersFound = 0;
      JSON::ObjectDecoder objectDecoder( entity );
      JSON::Entity keyString, valueEntity;
      while ( objectDecoder.getNext( keyString, valueEntity ) )
      {
        std::string name;
        name.resize( keyString.stringLength() );
        keyString.stringGetData( &name[0] );
        
        try
        {
          NameToIndexMap::const_iterator it = m_nameToIndexMap.find( name );
          if ( it == m_nameToIndexMap.end() )
          {
            // [pzion 20120124] For historical reasons we silently drop unknown members...
            //throw Exception("member not found");
            continue;
          }
          size_t memberIndex = it->second;
          void *memberData = static_cast<uint8_t *>(data) + m_memberOffsets[memberIndex];
          m_memberInfos[memberIndex].desc->decodeJSON( valueEntity, memberData );
        }
        catch ( Exception e )
        {
          throw _(name) + ": " + e;
        }

        ++membersFound;
      }
      
      if ( membersFound != m_numMembers )
        throw Exception( "missing members" );
    }

    void StructImpl::disposeDatasImpl( void *data, size_t count, size_t stride ) const
    {
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = m_memberInfos[i];
        void *memberData = static_cast<uint8_t *>(data) + m_memberOffsets[i];
        memberInfo.desc->disposeDatas( memberData, count, stride );
      }
    }
    
    std::string StructImpl::descData( void const *data ) const
    {
      std::string result = "{";
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = m_memberInfos[i];
        void const *memberData = static_cast<uint8_t const *>(data) + m_memberOffsets[i];
        if ( i > 0 )
          result += ",";
        result += memberInfo.name;
        result += ":";
        result += memberInfo.desc->descData( memberData );
      }
      result += "}";
      return result;
    }
    
    bool StructImpl::isShallow() const
    {
      return m_isShallow;
    }

    bool StructImpl::isNoAliasSafe() const
    {
      return m_isNoAliasSafe;
    }

    bool StructImpl::isEquivalentTo( RC::ConstHandle<Impl> const &thatImpl ) const
    {
      if ( !isStruct( thatImpl->getType() ) )
        return false;
      RC::ConstHandle<StructImpl> thatStructImpl = RC::ConstHandle<StructImpl>::StaticCast( thatImpl );
      
      size_t thisNumMembers = getNumMembers();
      size_t thatNumMembers = thatStructImpl->getNumMembers();
      if ( thisNumMembers != thatNumMembers )
        return false;
      
      for ( size_t i=0; i<thisNumMembers; ++i )
      {
        StructMemberInfo const &thisMemberInfo = getMemberInfo( i );
        StructMemberInfo const &thatMemberInfo = thatStructImpl->getMemberInfo( i );
        if ( thisMemberInfo.name != thatMemberInfo.name
          || !thisMemberInfo.desc->isEquivalentTo( thatMemberInfo.desc )
          //|| thisMemberInfo.defaultData != thatMemberInfo.defaultData
          )
          return false;
      }
      
      return true;
    }
    
    bool StructImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      if ( m_isShallow )
        return memcmp( lhs, rhs, getAllocSize() ) == 0;
      else
      {
        for ( size_t i=0; i<m_numMembers; ++i )
        {
          StructMemberInfo const &memberInfo = m_memberInfos[i];
          if( !memberInfo.desc->equalsData( getImmutableMemberData_NoCheck( lhs, i ), getImmutableMemberData_NoCheck( rhs, i ) ) )
            return false;
        }
        return true;
      }
    }

    size_t StructImpl::getIndirectMemoryUsage( void const *data ) const
    {
      size_t total = 0;
      for ( size_t i=0; i<m_numMembers; ++i )
        total += m_memberInfos[i].desc->getIndirectMemoryUsage( getImmutableMemberData_NoCheck( data, i ) );
      return total;
    }
    
    bool StructImpl::isExportable() const
    {
      return m_isExportable;
    }
  }
}
