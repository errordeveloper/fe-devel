/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/DG/ResourceLoadNode.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/RT/BooleanDesc.h>
#include <Fabric/Core/RT/ImplType.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Base/JSON/Value.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Integer.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<ResourceLoadNode> ResourceLoadNode::Create( std::string const &name, RC::Handle<Context> const &context )
    {
      RC::Handle<ResourceLoadNode> resourceLoadNode = new ResourceLoadNode( name, context );
      
      Util::SimpleString json;
      {
        Util::JSONGenerator jg( &json );
        resourceLoadNode->jsonDesc( jg );
      }
      resourceLoadNode->jsonNotifyDelta( json );

      return resourceLoadNode;
    }
    
    ResourceLoadNode::ResourceLoadNode( std::string const &name, RC::Handle<Context> const &context )
      : Node( name, context )
      , m_context( context.ptr() )
      , m_streamGeneration( 0 )
      , m_fabricResourceStreamData( context->getRTManager() )
      , m_nbStreamed( 0 )
      , m_keepMemoryCache( false )
      , m_firstEvalAfterLoad( false )
    {
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      RC::ConstHandle<RT::BooleanDesc> booleanDesc = context->getRTManager()->getBooleanDesc();

      addMember( "url", stringDesc, stringDesc->getDefaultData() );
      addMember( "resource", m_fabricResourceStreamData.getDesc(), m_fabricResourceStreamData.getDesc()->getDefaultData() );
      addMember( "keepMemoryCache", booleanDesc, booleanDesc->getDefaultData() );
      addMember( "asFile", booleanDesc, booleanDesc->getDefaultData() );
    }

    void ResourceLoadNode::jsonExecCreate(
      RC::ConstHandle<JSON::Value> const &arg,
      RC::Handle<Context> const &context,
      Util::JSONArrayGenerator &resultJAG
      )
    {
      Create( arg->toString()->value(), context );
    }

    void ResourceLoadNode::evaluateLocal( void *userdata )
    {
      if( isDirty() )
      {
        // [JeromeCG 20110727]Important: Url streaming task must be run in main thread only since it might use some thread-sensitive APIs such as NPAPI's stream interface
        MT::executeParallel( m_context->getLogCollector(), 1, &ResourceLoadNode::EvaluateResource, (void *)this, true );
      }
      Node::evaluateLocal( userdata );
    }

    void ResourceLoadNode::evaluateResource()
    {
      void const *urlMemberData = getConstData( "url", 0 );
      bool sameURL = m_fabricResourceStreamData.isURLEqualTo( urlMemberData );

      bool isFirstEvalAfterLoad = m_firstEvalAfterLoad;
      m_firstEvalAfterLoad = false;

      bool loadingFinished = !m_stream;
      if( sameURL && !loadingFinished )
        return;

      if( sameURL && isFirstEvalAfterLoad )
        return;//[JeromeCG 20111221] The data was already set asynchronously, during setResourceData, so the work is already done.

      if( sameURL && m_keepMemoryCache )
      {
        setResourceData( 0, false );
        return;
      }

      // [JeromeCG 20110727] Note: we use a generation because if there was a previous stream created for a previous URL we cannot destroy it; 
      // we create a new one in parallel instead of waiting its completion.
      m_streamGeneration++;
      m_fabricResourceStreamData.setURL( urlMemberData );
      m_fabricResourceStreamData.setMIMEType( "" );
      m_fabricResourceStreamData.resizeData( 0 );
      m_keepMemoryCache = false;

      setResourceData( 0, false );

      m_keepMemoryCache = getContext()->getRTManager()->getBooleanDesc()->getValue( getConstData( "keepMemoryCache", 0 ) );
      m_asFile = getContext()->getRTManager()->getBooleanDesc()->getValue( getConstData( "asFile", 0 ) );

      std::string url = m_fabricResourceStreamData.getURL();
      if( !url.empty() )
      {
        m_nbStreamed = 0;
        m_progressNotifTimer.reset();

        m_stream = getContext()->getIOManager()->createStream(
          url,
          m_asFile,
          &ResourceLoadNode::StreamData,
          &ResourceLoadNode::StreamEnd,
          &ResourceLoadNode::StreamFailure,
          this,
          (void*)m_streamGeneration
          );
      }
    }

    void ResourceLoadNode::streamData( std::string const &url, std::string const &mimeType, size_t totalsize, size_t offset, size_t size, void const *data, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;

      if( size )
      {
        if( !m_asFile )
        {
          size_t prevSize = m_fabricResourceStreamData.getDataSize();
          if ( offset + size > prevSize )
            m_fabricResourceStreamData.resizeData( offset + size );
          m_fabricResourceStreamData.setData( offset, size, data );
        }

        m_nbStreamed += size;
        int deltaMS = (int)m_progressNotifTimer.getElapsedMS(false);

        const int progressNotifMinMS = 200;

        if( deltaMS > progressNotifMinMS )
        {
          m_progressNotifTimer.reset();

          std::vector<std::string> src;
          src.push_back( "DG" );
          src.push_back( getName() );

          Util::SimpleString json;
          {
            Util::JSONGenerator jg( &json );
            Util::JSONObjectGenerator jog = jg.makeObject();
            {
              Util::JSONGenerator memberJG = jog.makeMember( "received", 8 );
              memberJG.makeInteger( m_nbStreamed );
            }
            {
              Util::JSONGenerator memberJG = jog.makeMember( "total", 5 );
              memberJG.makeInteger( totalsize );
            }
          }
          getContext()->jsonNotify( src, "resourceLoadProgress", 20, &json );
        }
      }
    }

    void ResourceLoadNode::streamEnd( std::string const &url, std::string const &mimeType, std::string const *fileName, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;
      m_fabricResourceStreamData.setMIMEType( mimeType );
      m_stream = NULL;//[JeromeCG 20111221] Important: set stream to NULL (loadingFinished status) since setResourceData's notifications can trigger an evaluation
      setResourceData( 0, true );
    }

    void ResourceLoadNode::streamFailure( std::string const &url, std::string const &errorDesc, void *userData )
    {
      if( (size_t)userData != m_streamGeneration )
        return;

      m_fabricResourceStreamData.resizeData( 0 );
      m_stream = NULL;
      setResourceData( &errorDesc, true );
    }

    void ResourceLoadNode::setResourceData(
      std::string const *errorDesc,
      bool notify,
      std::string const *fileName
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

      if( !m_fabricResourceStreamData.isEqualTo( getConstData( "resource", 0 ) ) )
      {
        void *resourceDataMember = getMutableData( "resource", 0 );
        m_fabricResourceStreamData.getDesc()->setData( m_fabricResourceStreamData.get(), resourceDataMember );

        if( !m_keepMemoryCache )
          m_fabricResourceStreamData.resizeData( 0 );

        if( errorDesc )
        {
          if ( getContext()->getLogCollector() )
          {
            getContext()->getLogCollector()->add( ( "ResourceLoadNode " + getName() + ": error loading " + url + ": " + *errorDesc ).c_str() );
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
    }
  };
};
