/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/GC/Container.h>
#include <Fabric/Core/JSON/CommandChannel.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace GC
  {
    Container::Container( JSON::CommandChannel *jsonCommandChannel )
      : m_mutex( "GC::Container" )
      , m_jsonCommandChannel( jsonCommandChannel )
    {
    }
      
    Container::~Container()
    {
      // [pzion 20111212] We could report leaks here, but it will be common
      // that scripts forget to call dispose() on their objects...
      IDToObjectMap::const_iterator it = m_idToObjectMap.begin();
      while ( it != m_idToObjectMap.end() )
      {
        IDToObjectMap::const_iterator thisIT = it++;
        thisIT->second->dispose();
      }
      FABRIC_ASSERT( m_idToObjectMap.empty() );
    }
      
    void Container::registerObject( std::string const &id_, Object *object )
    {
      Util::Mutex::Lock mutexLock( m_mutex );
      if ( m_idToObjectMap.find( id_ ) != m_idToObjectMap.end() )
        throw Exception( "GC::Object " + _(id_) + " already in use" );
      m_idToObjectMap[id_] = object;
    }
    
    RC::Handle<Object> Container::maybeGetObject( std::string const &id_ ) const
    {
      Util::Mutex::Lock mutexLock( m_mutex );
      RC::Handle<Object> result;
      IDToObjectMap::const_iterator it = m_idToObjectMap.find( id_ );
      if ( it != m_idToObjectMap.end() )
        result = it->second;
      return result;
    }
    
    RC::Handle<Object> Container::getObject( std::string const &id_ ) const
    {
      RC::Handle<Object> result = maybeGetObject( id_ );
      if ( !result )
        throw Exception( "GC::Object " + _(id_) + " does not exist or has already been disposed" );
      return result;
    }
    
    void Container::disposeObject( std::string const &id_, Object *object )
    {
      Util::Mutex::Lock mutexLock( m_mutex );
      IDToObjectMap::iterator it = m_idToObjectMap.find( id_ );
      if ( it == m_idToObjectMap.end() )
        throw Exception( "GC::Object " + _(id_) + " does not exist or has already been disposed" );
      FABRIC_ASSERT( it->second.ptr() == object );
      m_idToObjectMap.erase( it );
    }

    void Container::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      FABRIC_ASSERT( dst.size() - dstOffset > 0 );
      std::string const &id_ = dst[dstOffset].stringToStdString();
      getObject( id_ )->jsonRoute( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
    }
        
    void Container::jsonNotify(
      std::string const &id_,
      char const *cmdData,
      size_t cmdLength,
      Util::SimpleString const *argJSON
      )
    {
      std::vector<std::string> src;
      src.reserve( 2 );
      src.push_back( "GC" );
      src.push_back( id_ );
      m_jsonCommandChannel->jsonNotify( src, cmdData, cmdLength, argJSON );
    }
  }
}
