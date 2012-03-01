/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "NamedObject.h"
#include "Context.h"
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  

  namespace DG
  {
    NamedObject::NamedObject( std::string const &name, RC::Handle<Context> const &context )
      : CompiledObject( context )
      , m_context( context.ptr() )
    {
      if ( name.empty() )
        throw Exception( "name must be non-empty" );
        
      m_notificationSrc.push_back( "DG" );
      m_notificationSrc.push_back( name );
      
      Context::NamedObjectMap &namedObjectMap = context->getNamedObjectRegistry();
      bool insertResult = namedObjectMap.insert( Context::NamedObjectMap::value_type( name, this ) ).second;
      if ( !insertResult )
        throw Exception( "an object named "+_(name)+" already exists" );
    }
    
    std::string const &NamedObject::getName() const
    {
      return m_notificationSrc[1];
    }

    void NamedObject::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 0 )
        jsonExec( cmd, arg, resultArrayEncoder );
      else throw Exception( "unroutable" );
    }
      
    void NamedObject::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      throw Exception( "unknown command" );
    }
      
    void NamedObject::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      jsonDesc( resultObjectEncoder );
    }
    
    void NamedObject::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "type", 4 );
        jsonDescType( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "errors", 6 );
        jsonDescErrors( memberEncoder );
      }
    }
    
    void NamedObject::jsonNotify( char const *cmdData, size_t cmdLength, Util::SimpleString const *argJSON ) const
    {
      m_context->jsonNotify( m_notificationSrc, cmdData, cmdLength, argJSON );
    }
    
    void NamedObject::jsonNotifyDelta( Util::SimpleString const &delta ) const
    {
      jsonNotify( "delta", 5, &delta );
    }
    
    void NamedObject::jsonNotifyMemberDelta(
      char const *memberData,
      size_t memberLength,
      Util::SimpleString const &memberDeltaJSON
      ) const
    {
      Util::SimpleString json;
      {
        JSON::Encoder jsonEncoder( &json );
        JSON::ObjectEncoder jsonObjectEncoder = jsonEncoder.makeObject();
        JSON::Encoder deltaEncoder = jsonObjectEncoder.makeMember( memberData, memberLength );
        deltaEncoder.appendJSON( memberDeltaJSON );
      }
      jsonNotifyDelta( json );
    }
    
    void NamedObject::jsonNotifyErrorDelta() const
    {
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescErrors( jg );
      }
      jsonNotifyMemberDelta( "errors", 6, json );
    }

    void NamedObject::jsonGetMemoryUsage( JSON::Encoder &jg ) const
    {
      jg.makeInteger( 0 );
    }
  };
};
