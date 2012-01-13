/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/IO/Stream.h>
#include <Fabric/Core/Util/Assert.h>

namespace Fabric
{
  namespace IO
  {
    Stream::Stream(
      DataCallback dataCallback,
      EndCallback endCallback,
      FailureCallback failureCallback,
      RC::Handle<RC::Object> const &target,
      void *userData
      )
      : m_dataCallback( dataCallback )
      , m_endCallback( endCallback )
      , m_failureCallback( failureCallback )
      , m_target( target )
      , m_userData( userData )
    {
    }
    
    Stream::~Stream()
    {
    }
    
    void Stream::onData( std::string const &url, std::string const &mimeType, size_t totalsize, size_t offset, size_t size, void const *data )
    {
      FABRIC_ASSERT( m_target );
      m_dataCallback( url, mimeType, totalsize, offset, size, data, m_target.makeStrong(), m_userData );
    }

    void Stream::onEnd( std::string const &url, std::string const &mimeType, std::string const *fileName )
    {
      FABRIC_ASSERT( m_target );
      m_endCallback( url, mimeType, fileName, m_target.makeStrong(), m_userData );
      m_target = 0;
    }

    void Stream::onFailure( std::string const &url, std::string const &errorDesc )
    {
      FABRIC_ASSERT( m_target );
      m_failureCallback( url, errorDesc, m_target.makeStrong(), m_userData );
      m_target = 0;
    }
  };
};
