/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "HTTPResourceProvider.h"
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/RC/WeakHandle.h>
#include <Fabric/Base/Util/Assert.h>

namespace Fabric
{
  namespace NPAPI
  {
    RC::Handle<HTTPResourceProvider> HTTPResourceProvider::Create( NPP npp )
    {
      return new HTTPResourceProvider( npp );
    }

    HTTPResourceProvider::HTTPResourceProvider( NPP npp )
      : m_npp( npp )
    {
    }

    struct HTTPStream
    {
      std::string m_mimeType;
      bool m_asFile;
      void* m_userData;
      size_t m_nbReceived;
      bool m_finished;
    };

    char const *HTTPResourceProvider::getUrlScheme() const
    {
      return "http";//More is supported (like ftp), but we should be the default resource provider so all these should come our way
    }

    void HTTPResourceProvider::get( char const *url, bool getAsFile, void* userData )
    {
      //Don't validate the url ourselves; the browser supports multiple schemes, relative paths, etc.
      HTTPStream *requestStruct = new HTTPStream();
      requestStruct->m_asFile = getAsFile;
      requestStruct->m_userData = userData;
      requestStruct->m_nbReceived = 0;
      requestStruct->m_finished = false;

      NPError npError = NPN_GetURLNotify( m_npp, url, 0, requestStruct );
      if ( npError != NPERR_NO_ERROR )
        throw Exception( "Unable to complete URL request (NPN_GetURLNotify failed)" );
    }

    NPError HTTPResourceProvider::nppNewStream( NPP npp, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype )
    {
      FABRIC_ASSERT( npp == m_npp );

      HTTPStream *streamStruct = (HTTPStream*)stream->notifyData;
      streamStruct->m_mimeType = type;
      *stype = streamStruct->m_asFile ? NP_ASFILE : NP_NORMAL;
      return NPERR_NO_ERROR;
    }
    
    int32_t HTTPResourceProvider::nppWriteReady( NPP npp, NPStream* stream )
    {
      FABRIC_ASSERT( npp == m_npp );
      return 1<<17;//<= 128K buffers
    }

    int32_t HTTPResourceProvider::nppWrite( NPP npp, NPStream* stream, int32_t offset, int32_t len, void* buffer )
    {
      FABRIC_ASSERT( npp == m_npp );

      HTTPStream *streamStruct = (HTTPStream*)stream->notifyData;
      if( !streamStruct->m_asFile )
        IO::ResourceManager::onData( offset, len, buffer, streamStruct->m_userData );

      streamStruct->m_nbReceived += len;
      size_t total = stream->end;

      //Note: ensure that onProgress(100%) is issued after data is ready; if m_asFile we wait after the onFile call
      if( !streamStruct->m_asFile || streamStruct->m_nbReceived != total )
      {
        if( !streamStruct->m_finished )
        {
          IO::ResourceManager::onProgress( streamStruct->m_mimeType.c_str(), streamStruct->m_nbReceived, total, streamStruct->m_userData );
          streamStruct->m_finished = streamStruct->m_nbReceived == total;
        }
      }

      return len;
    }
    
    NPError HTTPResourceProvider::nppDestroyStream( NPP npp, NPStream *stream, NPReason reason )
    {
      FABRIC_ASSERT( npp == m_npp );
      HTTPStream *streamStruct = (HTTPStream*)stream->notifyData;
      switch ( reason )
      {
        case NPRES_DONE:
          //onProgress(100%) callback has already been called
          break;
        case NPRES_USER_BREAK:
          if(!streamStruct->m_finished)//might be called after all the data has been received...
            IO::ResourceManager::onFailure( "Cancelled by user", streamStruct->m_userData );
          break;
        case NPRES_NETWORK_ERR:
          if(!streamStruct->m_finished)//might be called after all the data has been received...
            IO::ResourceManager::onFailure( "Network error", streamStruct->m_userData );
          break;
      }
      delete streamStruct;
      return NPERR_NO_ERROR;
    }

    NPError HTTPResourceProvider::nppStreamAsFile( NPP npp, NPStream *stream, const char* fname )
    {
      FABRIC_ASSERT( npp == m_npp );

      HTTPStream *streamStruct = (HTTPStream*)stream->notifyData;
      IO::ResourceManager::onFile( fname, streamStruct->m_userData );
      size_t total = stream->end;
      IO::ResourceManager::onProgress( streamStruct->m_mimeType.c_str(), total, total, streamStruct->m_userData );
      streamStruct->m_finished = true;
      return NPERR_NO_ERROR;
    }
  };
};
