/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/FabricResource.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/RT/VariableArrayDesc.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Base/Util/Assert.h>

namespace Fabric
{
  namespace DG
  {
    enum FabricResourceMemberIndices
    {
      FABRIC_RESOURCE_DATA_MEMBER_INDEX,
      FABRIC_RESOURCE_MIMETYPE_MEMBER_INDEX,
      FABRIC_RESOURCE_EXTENSION_MEMBER_INDEX,
      FABRIC_RESOURCE_URL_MEMBER_INDEX,
      FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX,
      NB_FABRIC_RESOURCE_MEMBERS
    };

    void RegisterFabricResourceType( RC::Handle<RT::Manager> const &rtManager )
    {
      RT::StructMemberInfoVector memberInfos;
      RC::ConstHandle<RT::StringDesc> stringDesc = rtManager->getStringDesc();
      memberInfos.resize(NB_FABRIC_RESOURCE_MEMBERS);

      memberInfos[FABRIC_RESOURCE_DATA_MEMBER_INDEX].name = "data";
      memberInfos[FABRIC_RESOURCE_DATA_MEMBER_INDEX].desc = rtManager->getVariableArrayOf( rtManager->getByteDesc() );
      memberInfos[FABRIC_RESOURCE_MIMETYPE_MEMBER_INDEX].name = "mimeType";
      memberInfos[FABRIC_RESOURCE_MIMETYPE_MEMBER_INDEX].desc = stringDesc;
      memberInfos[FABRIC_RESOURCE_EXTENSION_MEMBER_INDEX].name = "extension";
      memberInfos[FABRIC_RESOURCE_EXTENSION_MEMBER_INDEX].desc = stringDesc;
      memberInfos[FABRIC_RESOURCE_URL_MEMBER_INDEX].name = "url";
      memberInfos[FABRIC_RESOURCE_URL_MEMBER_INDEX].desc = stringDesc;
      memberInfos[FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX].name = "dataExternalLocation";
      memberInfos[FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX].desc = stringDesc;

      rtManager->registerStruct( "FabricResource", memberInfos );
    }

    RC::ConstHandle<RT::StructDesc> getFabricResourceDesc( RC::ConstHandle<RT::Manager> const &rtManager )
    {
      RC::ConstHandle<RT::Desc> fabricResourceDesc = rtManager->getDesc("FabricResource");
      FABRIC_ASSERT( fabricResourceDesc && RT::isStruct( fabricResourceDesc->getType() ) );
      return RC::ConstHandle<RT::StructDesc>::StaticCast( fabricResourceDesc );
    }

    size_t getFabricResourceDataSize( RC::ConstHandle<RT::StructDesc> const &desc, RC::ConstHandle<RT::VariableArrayDesc> const &dataMemberDesc, const void* resource )
    {
      return dataMemberDesc->getImpl()->getNumMembers( desc->getImmutableMemberData( resource, FABRIC_RESOURCE_DATA_MEMBER_INDEX ) );
    }

    const void *getFabricResourceDataPtr( RC::ConstHandle<RT::StructDesc> const &desc, RC::ConstHandle<RT::VariableArrayDesc> const &dataMemberDesc, const void* resource )
    {
      // [JeromeCG 20110831] Question: is there a cleaner way to access the .data() like opaque adapters?
      size_t dataSize = getFabricResourceDataSize( desc, dataMemberDesc, resource );
      return dataSize == 0 ? NULL : dataMemberDesc->getImpl()->getImmutableMemberData( desc->getImmutableMemberData( resource, FABRIC_RESOURCE_DATA_MEMBER_INDEX ), 0 );
    }

    FabricResourceWrapper::FabricResourceWrapper( RC::ConstHandle<RT::Manager> rtManager, void *resourceToAttach )
      : m_resource( resourceToAttach )
      , m_alloc( resourceToAttach == NULL )
      , m_rtManager( rtManager )
    {
      m_desc = getFabricResourceDesc( m_rtManager );
      m_dataMemberDesc = RC::ConstHandle<RT::VariableArrayDesc>::StaticCast( m_desc->getMemberInfo( FABRIC_RESOURCE_DATA_MEMBER_INDEX ).desc );

      if( m_alloc )
      {
        size_t size = m_desc->getAllocSize();
        m_resource = malloc( size );
        memset( m_resource, 0, size );
        m_desc->setData( m_desc->getDefaultData(), m_resource );
      }
    }

    FabricResourceWrapper::~FabricResourceWrapper()
    {
      if( m_alloc )
      {
        m_desc->disposeData( m_resource );
        free( m_resource );
      }
    }

    RC::ConstHandle<RT::StructDesc> FabricResourceWrapper::getDesc() const
    {
      return m_desc;
    }

    bool FabricResourceWrapper::isDataEqualTo( const void *other ) const
    {
      return m_dataMemberDesc->equalsData( m_desc->getImmutableMemberData( m_resource, FABRIC_RESOURCE_DATA_MEMBER_INDEX), m_desc->getImmutableMemberData( other, FABRIC_RESOURCE_DATA_MEMBER_INDEX) );
    }

    bool FabricResourceWrapper::isDataExternalLocationEqualTo( const void *other ) const
    {
      return m_rtManager->getStringDesc()->equalsData( m_desc->getImmutableMemberData( m_resource, FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX), m_desc->getImmutableMemberData( other, FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX) );
    }

    bool FabricResourceWrapper::isURLEqualTo( const void *otherStringData ) const
    {
      return m_rtManager->getStringDesc()->equalsData( m_desc->getImmutableMemberData( m_resource, FABRIC_RESOURCE_URL_MEMBER_INDEX), otherStringData );
    }

    void FabricResourceWrapper::setExtension( std::string const &value )
    {
      setStringMember( FABRIC_RESOURCE_EXTENSION_MEMBER_INDEX, value );
    }

    std::string FabricResourceWrapper::getExtension() const
    {
      return getStringMember( FABRIC_RESOURCE_EXTENSION_MEMBER_INDEX );
    }

    void FabricResourceWrapper::setURL( std::string const &value )
    {
      setStringMember( FABRIC_RESOURCE_URL_MEMBER_INDEX, value );
    }

    void FabricResourceWrapper::setURL( void const *value )
    {
      m_rtManager->getStringDesc()->setData( value, m_desc->getMutableMemberData( m_resource, FABRIC_RESOURCE_URL_MEMBER_INDEX ) );
    }

    std::string FabricResourceWrapper::getURL() const
    {
      return getStringMember( FABRIC_RESOURCE_URL_MEMBER_INDEX );
    }

    std::string FabricResourceWrapper::getMIMEType() const
    {
      return getStringMember( FABRIC_RESOURCE_MIMETYPE_MEMBER_INDEX );
    }

    void FabricResourceWrapper::setMIMEType( std::string const &value )
    {
      setStringMember( FABRIC_RESOURCE_MIMETYPE_MEMBER_INDEX, value );
    }

    std::string FabricResourceWrapper::getDataExternalLocation() const
    {
      return getStringMember( FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX );
    }

    void FabricResourceWrapper::setDataExternalLocation( std::string const &value )
    {
      setStringMember( FABRIC_RESOURCE_DATAEXTERNALLOCATION_MEMBER_INDEX, value );
    }

    void FabricResourceWrapper::resizeData( size_t size )
    {
      m_dataMemberDesc->setNumMembers( m_desc->getMutableMemberData( m_resource, FABRIC_RESOURCE_DATA_MEMBER_INDEX ), size, NULL );
    }

    void FabricResourceWrapper::setData( size_t offset, size_t size, const void* data )
    {
      m_dataMemberDesc->getImpl()->setMembers( m_desc->getMutableMemberData( m_resource, FABRIC_RESOURCE_DATA_MEMBER_INDEX ), offset, size, data );
    }

    size_t FabricResourceWrapper::getDataSize() const
    {
      return getFabricResourceDataSize( m_desc, m_dataMemberDesc, m_resource );
    }

    const void *FabricResourceWrapper::getDataPtr() const
    {
      return getFabricResourceDataPtr( m_desc, m_dataMemberDesc, m_resource );
    }

    std::string FabricResourceWrapper::getStringMember( size_t index ) const
    {
      RC::ConstHandle<RT::StringDesc> stringDesc = m_rtManager->getStringDesc();
      const void* extension = m_desc->getImmutableMemberData( m_resource, index );
      return std::string( stringDesc->getValueData( extension ), stringDesc->getValueLength( extension ) );
    }

    void FabricResourceWrapper::setStringMember( size_t index, std::string const &value )
    {
      m_rtManager->getStringDesc()->setValue( value.data(), value.length(), m_desc->getMutableMemberData( m_resource, index ) );
    }
  };
};
