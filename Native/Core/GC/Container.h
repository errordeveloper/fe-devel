/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_GC_CONTAINER_H
#define _FABRIC_GC_CONTAINER_H

#include <Fabric/Core/GC/Object.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Base/RC/Handle.h>

#include <map>
#include <string>

namespace Fabric
{
  namespace JSON
  {
    class ArrayEncoder;
    class CommandChannel;
    struct Entity;
  }
  
  namespace GC
  {
    class Container
    {
      typedef std::map< std::string, RC::Handle<Object> > IDToObjectMap;
      
      friend class Object;
      
    public:
    
      Container( JSON::CommandChannel *jsonCommandChannel );
      ~Container();
      
      RC::Handle<Object> maybeGetObject( std::string const &id_ ) const;
      RC::Handle<Object> getObject( std::string const &id_ ) const;
      
      void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
        
      void jsonNotify(
        std::string const &id_,
        char const *cmdData,
        size_t cmdLength,
        Util::SimpleString const *argJSON
        );

    protected:
      
      void registerObject( std::string const &id_, Object *object );
      void disposeObject( std::string const &id_, Object *object );
      
    private:
    
      mutable Util::Mutex m_mutex;
      IDToObjectMap m_idToObjectMap;
      JSON::CommandChannel *m_jsonCommandChannel;
    };
  }
}

#endif //_FABRIC_GC_CONTAINER_H
