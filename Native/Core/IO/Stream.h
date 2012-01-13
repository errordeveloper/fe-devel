/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_STREAM_H
#define _FABRIC_IO_STREAM_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/WeakHandle.h>

#include <string>

namespace Fabric
{
  namespace IO
  {
    class Stream : public RC::Object
    {
    public:
    
      typedef void (*DataCallback)( std::string const &url, std::string const &mimeType, size_t totalsize, size_t offset, size_t size, void const *data, RC::Handle<RC::Object> const &target, void *userData );
      typedef void (*EndCallback)( std::string const &url, std::string const &mimeType, std::string const *fileName, RC::Handle<RC::Object> const &target, void *userData );
      typedef void (*FailureCallback)( std::string const &url, std::string const &errorDesc, RC::Handle<RC::Object> const &target, void *userData );

    protected:
    
      Stream(
        DataCallback dataCallback,
        EndCallback endCallback,
        FailureCallback failureCallback,
        RC::Handle<RC::Object> const &target,
        void *userData
        );
      ~Stream();
      
      void onData( std::string const &url, std::string const &mimeType, size_t totalsize, size_t offset, size_t size, void const *data );
      void onEnd( std::string const &url, std::string const &mimeType, std::string const *fileName );
      void onFailure( std::string const &url, std::string const &errorDesc );
    
    private:
    
      DataCallback m_dataCallback;
      EndCallback m_endCallback;
      FailureCallback m_failureCallback;
      RC::WeakHandle<RC::Object> m_target;
      void* m_userData;
    };
  };
};

#endif //_FABRIC_IO_STREAM_H
