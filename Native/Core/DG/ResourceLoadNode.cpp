/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/DG/ResourceLoadNode.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/RT/BooleanDesc.h>
#include <Fabric/Core/RT/ImplType.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Base/Util/Assert.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<ResourceLoadNode> ResourceLoadNode::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<ResourceLoadNode> resourceLoadNode = new ResourceLoadNode( name, context );
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        resourceLoadNode->jsonDesc( jg );
      }
      resourceLoadNode->jsonNotifyDelta( json );

      return resourceLoadNode;
    }
    
    ResourceLoadNode::ResourceLoadNode( std::string const &name, RC::Handle<Context> const &context )
      : Node( name, context )
      , m_fabricResourceStreamData( context->getRTManager() )
      , m_firstEvalAfterLoad( false )
      , m_keepMemoryCache( false )
      , m_asFile( false )
      , m_inProgress( false )
      , m_streamGeneration( 0 )
    {
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      RC::ConstHandle<RT::BooleanDesc> booleanDesc = context->getRTManager()->getBooleanDesc();

      addMember( "url", stringDesc, stringDesc->getDefaultData() );
      addMember( "resource", m_fabricResourceStreamData.getDesc(), m_fabricResourceStreamData.getDesc()->getDefaultData() );
      addMember( "keepMemoryCache", booleanDesc, booleanDesc->getDefaultData() );
      addMember( "storeDataAsFile", booleanDesc, booleanDesc->getDefaultData() );
    }

    void ResourceLoadNode::jsonExecCreate(
      JSON::Entity const &arg,
      RC::Handle<Context> const &context,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      arg.requireString();
      Create( arg.stringToStdString(), context );
    }

    void ResourceLoadNode::evaluateLocal( void *userdata )
    {
      if( isDirty() )
      {
        // [JeromeCG 20110727] Important: Url streaming task must be run in main thread only since it might use some thread-sensitive APIs such as NPAPI's stream interface
        MT::executeParallel( getContext()->getLogCollector(), 1, &ResourceLoadNode::EvaluateResource, (void *)this, true );
      }
      Node::evaluateLocal( userdata );
    }

    void ResourceLoadNode::evaluateResource()
    {
      void const *urlMemberData = getConstData( "url", 0 );
      bool sameURL = m_fabricResourceStreamData.isURLEqualTo( urlMemberData );

      bool isFirstEvalAfterLoad = m_firstEvalAfterLoad;
      m_firstEvalAfterLoad = false;

      if( sameURL && m_inProgress )
        return;

      if( sameURL && isFirstEvalAfterLoad )
        return;//[JeromeCG 20111221] The data was already set asynchronously, during setResourceData, so the work is already done.

      if( sameURL && (m_keepMemoryCache || m_asFile) )
      {
        //Set from m_fabricResourceStreamData
        //[JeromeCG 20110727] Note: if m_asFile, the handle was created as "readOnly", so in theory the data is still valid.
        setResourceData( NULL, false );
        return;
      }

      // [JeromeCG 20110727] Note: we use a generation because if there was a previous uncompleted request we might receive more callbacks; 
      // we create a new one in parallel instead of waiting its completion.
      m_streamGeneration++;
      m_fabricResourceStreamData.setURL( urlMemberData );
      m_fabricResourceStreamData.setMIMEType( "" );
      //[JeromeCG 20120119] setDataExternalLocation: Should we care about deleting the cache file? The browser created it... can it reuse the same temp file? Maybe the user still uses it?
      m_fabricResourceStreamData.setDataExternalLocation( "" );
      m_fabricResourceStreamData.resizeData( 0 );
      m_keepMemoryCache = false;
      setResourceData( NULL, false );

      m_keepMemoryCache = getContext()->getRTManager()->getBooleanDesc()->getValue( getConstData( "keepMemoryCache", 0 ) );
      m_asFile = getContext()->getRTManager()->getBooleanDesc()->getValue( getConstData( "storeDataAsFile", 0 ) );

      std::string url = m_fabricResourceStreamData.getURL();
      if( !url.empty() )
      {
        m_inProgress = true;
        getContext()->getIOManager()->getResourceManager()->get( url.c_str(), this, m_asFile, (void*)m_streamGeneration );
      }
      else
      {
        if(m_filePinning.is_open())
          m_filePinning.close();
      }
    }

    void ResourceLoadNode::onData( size_t offset, size_t size, void const *data, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;

      FABRIC_ASSERT( !m_asFile );
      if( size )
      {
        size_t prevSize = m_fabricResourceStreamData.getDataSize();
        if ( offset + size > prevSize )
          m_fabricResourceStreamData.resizeData( offset + size );
        m_fabricResourceStreamData.setData( offset, size, data );
      }
    }

    void ResourceLoadNode::onFile( char const *fileName, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;

      FABRIC_ASSERT( m_asFile );
      std::string handle = getContext()->getIOManager()->getFileHandleManager()->createHandle( fileName, false, true );
      m_fabricResourceStreamData.setDataExternalLocation( handle );
      if(m_filePinning.is_open())
        m_filePinning.close();
      try
      {
        m_filePinning.open( fileName, std::fstream::in | std::fstream::binary );
      }
      catch(...)
      {
        throw Exception( "Unable to open tempory file containing data for " + m_fabricResourceStreamData.getURL() );
      }
    }

    void ResourceLoadNode::onFailure( char const *errorDesc, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;

      m_fabricResourceStreamData.resizeData( 0 );
      m_fabricResourceStreamData.setDataExternalLocation( "" );
      m_inProgress = false;
      setResourceData( errorDesc, true );
    }

    void ResourceLoadNode::onProgress( char const *mimeType, size_t done, size_t total, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;

      size_t prevSize = m_fabricResourceStreamData.getDataSize();
      if ( total < prevSize )
        m_fabricResourceStreamData.resizeData( total );

      if( done < total )
      {
        std::vector<std::string> src;
        src.push_back( "DG" );
        src.push_back( getName() );

        Util::SimpleString json;
        {
            JSON::Encoder jsonEncoder( &json );
            JSON::ObjectEncoder jsonObjectEncoder = jsonEncoder.makeObject();
          {
              JSON::Encoder memberEncoder = jsonObjectEncoder.makeMember( "received", 8 );
              memberEncoder.makeInteger( done );
          }
          {
              JSON::Encoder memberEncoder = jsonObjectEncoder.makeMember( "total", 5 );
              memberEncoder.makeInteger( total );
          }
        }
        getContext()->jsonNotify( src, "resourceLoadProgress", 20, &json );
      }
      else
      {
        m_fabricResourceStreamData.setMIMEType( mimeType );
        m_inProgress = false;//[JeromeCG 20111221] Important: set m_inProgress to false since setResourceData's notifications can trigger an evaluation
        setResourceData( NULL, true );
      }
    }

    void ResourceLoadNode::retain() const
    {
      Node::retain();
    }

    void ResourceLoadNode::release() const
    {
      Node::release();
    }

    void ResourceLoadNode::setResourceData(
      char const *errorDesc,
      bool notify
      )
    {
      m_firstEvalAfterLoad = true;
      std::string url = m_fabricResourceStreamData.getURL();
      std::string extension = IO::GetURLExtension( url );
      size_t extensionPos = url.rfind('.');
      if( extensionPos != std::string::npos )
        extension = url.substr( extensionPos+1 );
      else
      {
        std::string mimeType = m_fabricResourceStreamData.getMIMEType();
        extensionPos = mimeType.rfind('/');
        if( extensionPos != std::string::npos )
          extension = mimeType.substr( extensionPos+1 );
      }
      m_fabricResourceStreamData.setExtension( extension );

      void const *prevResourceData = getConstData( "resource", 0 );
      if( !m_fabricResourceStreamData.isDataEqualTo( prevResourceData ) || !m_fabricResourceStreamData.isDataExternalLocationEqualTo( prevResourceData ) )
      {
        void *resourceDataMember = getMutableData( "resource", 0 );
        m_fabricResourceStreamData.getDesc()->setData( m_fabricResourceStreamData.get(), resourceDataMember );

        if( !m_keepMemoryCache )
          m_fabricResourceStreamData.resizeData( 0 );

        if( errorDesc )
        {
          if ( getContext()->getLogCollector() )
          {
            getContext()->getLogCollector()->add( ( "ResourceLoadNode " + getName() + ": error loading " + url + ": " + std::string( errorDesc ) ).c_str() );
          }
        }
      }
      if( notify )
      {
        std::vector<std::string> src;
        src.push_back( "DG" );
        src.push_back( getName() );

        if( errorDesc )
          getContext()->jsonNotify( src, "resourceLoadFailure", 19 );
        else
          getContext()->jsonNotify( src, "resourceLoadSuccess", 19 );
      }
    }
  };
};
