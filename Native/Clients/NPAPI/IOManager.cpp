/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Clients/NPAPI/IOManager.h>
#include <Fabric/Clients/NPAPI/IOStream.h>
#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/Util/Assert.h>
#include <npapi/npapi.h>
#include "HTTPResourceProvider.h"

namespace Fabric
{
  namespace NPAPI
  {
    RC::Handle<IOManager> IOManager::Create( NPP npp )
    {
      return new IOManager( npp );
    }

    void scheduleAsynchCallback( void* scheduleUserData, void (*callbackFunc)(void *), void *callbackFuncUserData )
    {
      NPN_PluginThreadAsyncCall( (NPP)scheduleUserData, callbackFunc, callbackFuncUserData );
    }
  
    IOManager::IOManager( NPP npp )
      : IO::Manager( scheduleAsynchCallback, npp )
      , m_npp( npp )
      , m_context( NULL )
      , m_httpResourceProvider( HTTPResourceProvider::Create( npp ) )
    {
      getResourceManager()->registerProvider( RC::Handle<IO::ResourceProvider>::StaticCast( httpResourceProvider ), true );
    }

    RC::Handle< HTTPResourceProvider > IOManager::getHTTPResourceProvider() const
    {
      return ;
    }

    void IOManager::setContext( RC::Handle<Context> const &context )
    {
      m_context = context.ptr();
    }
    
    RC::Handle<IO::Stream> IOManager::createStream(
      std::string const &url,
      bool asFile,
      IO::Stream::DataCallback dataCallback,
      IO::Stream::EndCallback endCallback,
      IO::Stream::FailureCallback failureCallback,
      RC::Handle<RC::Object> const &target,
      void *userData
      ) const
    {
      return IOStream::Create( m_npp, url, asFile, dataCallback, endCallback, failureCallback, target, userData );
    }
  
    NPError IOManager::nppNewStream( NPP npp, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype )
    {
      FABRIC_ASSERT( npp == m_npp );
      return static_cast<IOStream *>( stream->notifyData )->nppNewStream( npp, type, stream, seekable, stype );
    }
    
    int32_t IOManager::nppWriteReady( NPP npp, NPStream* stream )
    {
      FABRIC_ASSERT( npp == m_npp );
      return static_cast<IOStream *>( stream->notifyData )->nppWriteReady( npp, stream );
    }

    int32_t IOManager::nppWrite( NPP npp, NPStream* stream, int32_t offset, int32_t len, void* buffer )
    {
      FABRIC_ASSERT( npp == m_npp );
      return static_cast<IOStream *>( stream->notifyData )->nppWrite( npp, stream, offset, len, buffer );
    }
    
    NPError IOManager::nppDestroyStream( NPP npp, NPStream *stream, NPReason reason )
    {
      FABRIC_ASSERT( npp == m_npp );
      return static_cast<IOStream *>( stream->notifyData )->nppDestroyStream( npp, stream, reason );
    }

    NPError IOManager::nppStreamAsFile( NPP npp, NPStream *stream, const char* fname )
    {
      FABRIC_ASSERT( npp == m_npp );
      return static_cast<IOStream *>( stream->notifyData )->nppStreamAsFile( npp, stream, fname );
    }

    std::string IOManager::queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension ) const
    {
      FABRIC_ASSERT( m_context );
      return m_context->queryUserFilePath( existingFile, title, defaultFilename, extension );
    }
  };
};
