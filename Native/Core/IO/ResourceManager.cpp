/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <vector>

#include "ResourceManager.h"

#include <Fabric/Base/Exception.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Base/RC/WeakHandle.h>

namespace Fabric
{
  namespace IO
  {
    struct PendingRequestInfo
    {
      ResourceClient* m_client;
      void* m_clientUserData;
      std::string m_url;
      Util::Timer m_lastProgressTimer;
      RC::WeakHandle< IO::ResourceManager > m_manager;
      std::list< void* >::iterator m_pendingRequestsListIter;
    };

    RC::Handle<ResourceManager> ResourceManager::Create( ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData, float progressMaxFrequencySeconds )
    {
      return new ResourceManager( scheduleFunc, scheduleFuncUserData, progressMaxFrequencySeconds );
    }

    ResourceManager::ResourceManager( ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData, float progressMaxFrequencySeconds )
      : m_scheduleFunc(scheduleFunc)
      , m_scheduleFuncUserData(scheduleFuncUserData)
      , m_progressMaxFrequencyMS( size_t( progressMaxFrequencySeconds*1000 ) )
    {
    }

    ResourceManager::~ResourceManager()
    {
      releaseAllFiles();
      while( !m_pendingRequests.empty() )
      {
        PendingRequestInfo* requestInfo = (PendingRequestInfo*)m_pendingRequests.front();
        //Note: we don't delete the PendingRequestInfo* structs as there might be some pending async calls; leak instead of crash in this exceptional race condition.
        //[JeromeCG 20111221] Calling onFailure here is unsafe; might try to fire back a notif in the DG context... Commenting out until a good solution...
        //requestInfo->m_client->onFailure( ("Resource request for \"" + requestInfo->m_url + "\" failed because of termination").c_str(), requestInfo->m_clientUserData );
        requestInfo->m_client->release();
        m_pendingRequests.pop_front();
      }
    }

    void ResourceManager::registerProvider( RC::Handle<ResourceProvider> const &provider, bool setAsDefault )
    {
      std::string scheme = provider->getUrlScheme();

      std::pair<SchemeToProviderMap::iterator, bool> result = m_schemeToProvider.insert( std::make_pair(scheme, provider) );
      if( result.second && result.first->second != provider )
        throw Exception( "Error: another resource provider is already registered for URL scheme " + scheme );

      if( setAsDefault )
      {
        if( m_defaultProvider && m_defaultProvider != provider )
          throw Exception( "Error: there is already another default provider" );
        m_defaultProvider = provider;
      }
    }

    void ResourceManager::get( char const *url, ResourceClient* client, bool getAsFile, void* userData )
    {
      PendingRequestInfo* requestInfo = new PendingRequestInfo();
      requestInfo->m_client = client;
      requestInfo->m_clientUserData = userData;
      requestInfo->m_manager = this;
      requestInfo->m_url = url;
      requestInfo->m_lastProgressTimer.reset();

      client->retain();
      m_pendingRequests.push_front( requestInfo );
      requestInfo->m_pendingRequestsListIter = m_pendingRequests.begin();

      try
      {
        ResourceProvider* provider = m_defaultProvider.ptr();
        char const *separator = url;
        int separatorPos = -1;
        while( *separator )
        {
          if( *separator == '/')
            break;
          if( *separator == ':')
          {
            separatorPos = separator-url;
            break;
          }
          ++separator;
        }
        if( separatorPos != -1 )
        {
          std::string scheme(url, separatorPos);
          SchemeToProviderMap::iterator providerIt = m_schemeToProvider.find( scheme );
          if( providerIt != m_schemeToProvider.end() )
            provider = providerIt->second.ptr();
        }
        if( !provider )
          throw Exception( "No suitable provider registered" );
        else
          provider->get( url, getAsFile, requestInfo );
      }
      //Simplify error handling on client side: turn sync errors into async failure callbacks
      catch ( Exception e )
      {
        onFailureAsyncThreadCall( "Error: " + e, requestInfo );
      }
      catch ( ... )
      {
        onFailureAsyncThreadCall( "Unknown error", requestInfo );
      }
    }

    void ResourceManager::onCompletedRequest( void *userData )
    {
      PendingRequestInfo* requestInfo = (PendingRequestInfo*)userData;
      m_pendingRequests.erase( requestInfo->m_pendingRequestsListIter );
      requestInfo->m_client->release();
      delete requestInfo;
    }

    void ResourceManager::onProgress( char const *mimeType, size_t done, size_t total, void *userData )
    {
      PendingRequestInfo* requestInfo = (PendingRequestInfo*)userData;
      if( !requestInfo->m_manager )
        return;//Manager might have been destroyed, in which case we should ignore this call

      RC::Handle<ResourceManager> keepAlive( requestInfo->m_manager.makeStrong() );
      if( done < total)
      {
        int deltaMS = (int)(requestInfo->m_lastProgressTimer.getElapsedMS(false));
        if( deltaMS < (int)requestInfo->m_manager.makeStrong()->m_progressMaxFrequencyMS )
          return;
      }
      requestInfo->m_lastProgressTimer.reset();

      try
      {
        requestInfo->m_client->onProgress( mimeType, done, total, requestInfo->m_clientUserData );
      }
      catch ( Exception e )
      {
        FABRIC_LOG( "ResourceManager: error while calling client's onProgress for \"" + requestInfo->m_url + "\": " + e.getDesc() );
      }
      catch ( ... )
      {
        FABRIC_LOG( "ResourceManager: error while calling client's onProgress for \"" + requestInfo->m_url + "\"." );
      }

      if(done == total)
        requestInfo->m_manager.makeStrong()->onCompletedRequest( userData );
    }

    void ResourceManager::onData( size_t offset, size_t size, void const *data, void *userData )
    {
      PendingRequestInfo* requestInfo = (PendingRequestInfo*)userData;
      if( !requestInfo->m_manager )
        return;//Manager might have been destroyed, in which case we should ignore this call

      RC::Handle<ResourceManager> keepAlive( requestInfo->m_manager.makeStrong() );
      try
      {
        requestInfo->m_client->onData( offset, size, data, requestInfo->m_clientUserData );
      }
      catch ( Exception e )
      {
        FABRIC_LOG( "ResourceManager: error while calling client's onData for \"" + requestInfo->m_url + "\": " + e.getDesc() );
      }
      catch ( ... )
      {
        FABRIC_LOG( "ResourceManager: error while calling client's onData for \"" + requestInfo->m_url + "\"." );
      }
    }

    void ResourceManager::onFile( char const *fileName, void *userData )
    {
      PendingRequestInfo* requestInfo = (PendingRequestInfo*)userData;
      if( !requestInfo->m_manager )
        return;//Manager might have been destroyed, in which case we should ignore this call

      RC::Handle<ResourceManager> manager( requestInfo->m_manager.makeStrong() );
      FilesInUseMap::iterator it = manager->m_filesInUse.insert( std::make_pair( fileName, fopen( fileName, "rb" ) ) );
      it->second = fopen( fileName, "rb" );
      if( it->second == NULL )
      {
        manager->m_filesInUse.erase( it );
        throw Exception( "Unable to open tempory file containing data for " + requestInfo->m_url );
      }

      try
      {
        requestInfo->m_client->onFile( fileName, requestInfo->m_clientUserData );
      }
      catch ( Exception e )
      {
        manager->releaseFile( fileName );//[JeromeCG 20120223] Note: client might have called releaseFile, but it can be safely called multiple times
        FABRIC_LOG( "ResourceManager: error while calling client's onFile for \"" + requestInfo->m_url + "\": " + e.getDesc() );
      }
      catch ( ... )
      {
        manager->releaseFile( fileName );
        FABRIC_LOG( "ResourceManager: error while calling client's onFile for \"" + requestInfo->m_url + "\"." );
      }
    }

    void ResourceManager::onFailure( char const *errorDesc, void *userData )
    {
      PendingRequestInfo* requestInfo = (PendingRequestInfo*)userData;
      if( !requestInfo->m_manager )
        return;//Manager might have been destroyed, in which case we should ignore this call

      RC::Handle<ResourceManager> keepAlive( requestInfo->m_manager.makeStrong() );
      try
      {
        requestInfo->m_client->onFailure( ( "Error while processing request for URL \"" + requestInfo->m_url + "\": " + errorDesc).c_str(), requestInfo->m_clientUserData );
      }
      catch ( Exception e )
      {
        FABRIC_LOG( "ResourceManager: error while calling client's onFailure for \"" + requestInfo->m_url + "\": " + e.getDesc() );
      }
      catch ( ... )
      {
        FABRIC_LOG( "ResourceManager: error while calling client's onFailure for \"" + requestInfo->m_url + "\"." );
      }

      requestInfo->m_manager.makeStrong()->onCompletedRequest( userData );
    }

    struct OnProgressCallbackStruct
    {
      std::string m_mimeType;
      size_t m_done;
      size_t m_total;
      void *m_userData;
    };

    void OnProgressSyncCallback( void *userData )
    {
      OnProgressCallbackStruct* callStruct = (OnProgressCallbackStruct*)userData;
      ResourceManager::onProgress( callStruct->m_mimeType.c_str(), callStruct->m_done, callStruct->m_total, callStruct->m_userData );
      delete callStruct;
    }

    void ResourceManager::onProgressAsyncThreadCall( char const *mimeType, size_t done, size_t total, void *userData )
    {
      OnProgressCallbackStruct* callStruct = new OnProgressCallbackStruct();
      callStruct->m_mimeType = mimeType;
      callStruct->m_done = done;
      callStruct->m_total = total;
      callStruct->m_userData = userData;
      RC::Handle<ResourceManager> manager = ((PendingRequestInfo*)userData)->m_manager.makeStrong();
      (*( manager->m_scheduleFunc ) )( manager->m_scheduleFuncUserData, OnProgressSyncCallback, callStruct);
    }

    struct OnDataCallbackStruct
    {
      size_t m_offset;
      std::vector<char> m_data;
      void *m_userData;
    };

    void OnDataSyncCallback( void *userData )
    {
      OnDataCallbackStruct* callStruct = (OnDataCallbackStruct*)userData;
      ResourceManager::onData( callStruct->m_offset, callStruct->m_data.size(), callStruct->m_data.empty() ? NULL : &callStruct->m_data.front(), callStruct->m_userData );
      delete callStruct;
    }

    void ResourceManager::onDataAsyncThreadCall( size_t offset, size_t size, void const *data, void *userData )
    {
      OnDataCallbackStruct* callStruct = new OnDataCallbackStruct();
      callStruct->m_offset = offset;
      if(size)
      {
        callStruct->m_data.resize(size);
        memcpy( &callStruct->m_data.front(), data, size );
      }
      callStruct->m_userData = userData;
      RC::Handle<ResourceManager> manager = ((PendingRequestInfo*)userData)->m_manager.makeStrong();
      (*( manager->m_scheduleFunc ) )( manager->m_scheduleFuncUserData, OnDataSyncCallback, callStruct);
    }

    struct OnStringCallbackStruct
    {
      std::string m_string;
      void *m_userData;
    };

    void OnFileSyncCallback( void *userData )
    {
      OnStringCallbackStruct* callStruct = (OnStringCallbackStruct*)userData;
      ResourceManager::onFile( callStruct->m_string.c_str(), callStruct->m_userData );
      delete callStruct;
    }

    void ResourceManager::onFileAsyncThreadCall( char const *fileName, void *userData )
    {
      OnStringCallbackStruct* callStruct = new OnStringCallbackStruct();
      callStruct->m_string = fileName;
      callStruct->m_userData = userData;
      RC::Handle<ResourceManager> manager = ((PendingRequestInfo*)userData)->m_manager.makeStrong();
      (*( manager->m_scheduleFunc ) )( manager->m_scheduleFuncUserData, OnFileSyncCallback, callStruct);
    }

    void OnFailureSyncCallback( void *userData )
    {
      OnStringCallbackStruct* callStruct = (OnStringCallbackStruct*)userData;
      ResourceManager::onFailure( callStruct->m_string.c_str(), callStruct->m_userData );
      delete callStruct;
    }

    void ResourceManager::onFailureAsyncThreadCall( char const *errorDesc, void *userData )
    {
      OnStringCallbackStruct* callStruct = new OnStringCallbackStruct();
      callStruct->m_string = errorDesc;
      callStruct->m_userData = userData;
      RC::Handle<ResourceManager> manager = ((PendingRequestInfo*)userData)->m_manager.makeStrong();
      (*( manager->m_scheduleFunc ) )( manager->m_scheduleFuncUserData, OnFailureSyncCallback, callStruct);
    }


    void ResourceManager::releaseFile( char const *fileName )
    {
      FilesInUseMap::iterator it = m_filesInUse.find( fileName );
      if( it != m_filesInUse.end() )
      {
        fclose( it->second );
        m_filesInUse.erase( it );
      }
    }

    void ResourceManager::releaseAllFiles()
    {
      for( FilesInUseMap::iterator it = m_filesInUse.begin(); it != m_filesInUse.end(); ++it )
        fclose( it->second );

      m_filesInUse.clear();
    }
  };
};
