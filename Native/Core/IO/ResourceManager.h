/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_RESOURCEMANAGER_H
#define _FABRIC_IO_RESOURCEMANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <map>
#include <list>
#include <string>
#include <stdio.h>

namespace Fabric
{

  namespace IO
  {
    class ResourceManager;

    struct ResourceClient
    {
      //Note: the last issued call is either onProgress() with 100% done or an onFailure()
      virtual void onProgress( char const *mimeType, size_t done, size_t total, void *userData ) = 0;
      virtual void onData( size_t offset, size_t size, void const *data, void *userData ) = 0;
      virtual void onFile( char const *fileName, void *userData ) = 0;
      virtual void onFailure( char const *errorDesc, void *userData ) = 0;

      virtual void retain() const = 0;
      virtual void release() const = 0;
    };

    typedef void (*ScheduleAsyncCallbackFunc)( void* scheduleUserData, void (*callbackFunc)(void *), void *callbackFuncUserData );

    class ResourceProvider : public RC::Object
    {
    public:
      REPORT_RC_LEAKS

      virtual char const * getUrlScheme() const = 0;
      virtual void get( char const *url, bool getAsFile, void* userData ) = 0;
    };

    class ResourceManager : public RC::Object
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<ResourceManager> Create( ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData, float progressMaxFrequencySeconds = 0.3f );

      void registerProvider( RC::Handle<ResourceProvider> const &provider, bool setAsDefault = false );
      void get( char const *url, ResourceClient* client, bool getAsFile, void* userData );

      void releaseFile( char const *fileName );
      void releaseAllFiles();

      static void onProgress( char const *mimeType, size_t done, size_t total, void *userData );
      static void onData( size_t offset, size_t size, void const *data, void *userData );
      static void onFile( char const *fileName, void *userData );
      static void onFailure( char const *errorDesc, void *userData );

      //Async, inter-thread call scheduling helpers
      static void onProgressAsyncThreadCall( char const *mimeType, size_t done, size_t total, void *userData );
      static void onDataAsyncThreadCall( size_t offset, size_t size, void const *data, void *userData );
      static void onFileAsyncThreadCall( char const *fileName, void *userData );
      static void onFailureAsyncThreadCall( char const *errorDesc, void *userData );

    private:

      ResourceManager( ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData, float progressMaxFrequencySeconds );
      ~ResourceManager();

      void onCompletedRequest( void *userData );

      typedef std::map<std::string, RC::Handle<ResourceProvider> > SchemeToProviderMap;
      SchemeToProviderMap m_schemeToProvider;

      RC::Handle<ResourceProvider> m_defaultProvider;

      std::list< void* > m_pendingRequests;

      ScheduleAsyncCallbackFunc m_scheduleFunc;
      void *m_scheduleFuncUserData;
      size_t m_progressMaxFrequencyMS;

      typedef std::multimap<std::string, FILE* > FilesInUseMap;
      FilesInUseMap m_filesInUse;
    };
  };
};

#endif //_FABRIC_IO_RESOURCEMANAGER_H
