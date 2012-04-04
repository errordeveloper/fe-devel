/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/StructImpl.h>
 
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <set>
 
namespace Fabric
{
  namespace RT
  {
    StructImpl::StructImpl(
      std::string const &codeName,
      StructMemberInfoVector const &memberInfos
      )
      : m_memberInfos( memberInfos )
      , m_numMembers( memberInfos.size() )
      , m_defaultData( 0 )
    {
      size_t flags = FlagShallow | FlagExportable;
      m_memberOffsets.push_back( 0 );
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = getMemberInfo(i);
        m_memberOffsets.push_back( m_memberOffsets.back() + memberInfo.desc->getAllocSize() );
        m_nameToIndexMap.insert( NameToIndexMap::value_type( memberInfo.name, i ) );
        if ( !memberInfo.desc->isShallow() )
          flags &= ~FlagShallow;
        if ( memberInfo.desc->isNoAliasUnsafe() )
          flags |= FlagNoAliasUnsafe;
        if ( !memberInfo.desc->isExportable() )
          flags &= ~FlagExportable;
      }
      size_t size = m_memberOffsets[m_numMembers];

      initialize( codeName, DT_STRUCT, size, flags );

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
    
    void StructImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = m_memberInfos[i];
        size_t memberOffset = m_memberOffsets[i];
        void const *srcMemberData;
        if ( src )
          srcMemberData = src + memberOffset;
        else
          srcMemberData = 0;
        void *dstMemberData = dst + memberOffset;
        memberInfo.desc->initializeDatas( count, srcMemberData, srcStride, dstMemberData, dstStride );
      }
    }
    
    void StructImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = m_memberInfos[i];
        size_t memberOffset = m_memberOffsets[i];
        void const *srcMemberData = static_cast<uint8_t const *>(src) + memberOffset;
        void *dstMemberData = static_cast<uint8_t *>(dst) + memberOffset;
        memberInfo.desc->setDatas( count, srcMemberData, srcStride, dstMemberData, dstStride );
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
        
      std::set<std::string> membersFound;
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

          membersFound.insert( name );
        }
        catch ( Exception e )
        {
          throw _(name) + ": " + e;
        }
      }
      
      if ( membersFound.size() != m_numMembers )
      {
        std::string missingMembers = "";
        for ( NameToIndexMap::const_iterator it = m_nameToIndexMap.begin(); it != m_nameToIndexMap.end(); ++it )
        {
          if ( membersFound.find( it->first ) == membersFound.end() )
          {
            if ( !missingMembers.empty() )
              missingMembers += ", ";
            missingMembers += _(it->first);
          }
        }
        if ( membersFound.size() > 1 )
          throw Exception( "missing members " + missingMembers );
        else
          throw Exception( "missing member " + missingMembers );
      }
    }

    void StructImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      for ( size_t i=0; i<m_numMembers; ++i )
      {
        StructMemberInfo const &memberInfo = m_memberInfos[i];
        void *memberData = data + m_memberOffsets[i];
        memberInfo.desc->disposeDatas( count, memberData, stride );
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
      if ( isShallow() )
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
  }
}
