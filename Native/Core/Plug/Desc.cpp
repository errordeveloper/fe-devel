/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Desc.h"

#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace Plug
  {
    static HostSpec parseHostSpec( JSON::Entity const &entity )
    {
      HostSpec result;
      if ( entity.isString() )
        result.push_back( entity.stringToStdString() );
      else if ( entity.isArray() )
      {
        JSON::ArrayDecoder arrayDecoder( entity );
        JSON::Entity elementEntity;
        while ( arrayDecoder.getNext( elementEntity ) )
        {
          try
          {
            elementEntity.requireString();
            result.push_back( elementEntity.stringToStdString() );
          }
          catch ( Exception e )
          {
            arrayDecoder.rethrow( e );
          }
        }
      }
      else
        throw Exception( "must be a string or an array" );
      return result;
    }
    
    static void parseHostStrings( JSON::Entity const &entity, HostStrings &result )
    {
      if ( entity.isString() )
      {
        HostSpec hostSpec;
        hostSpec.push_back( "*" );
        result.insert( HostStrings::value_type( entity.stringToStdString(), hostSpec ) );
      }
      else if ( entity.isObject() )
      {
        JSON::ObjectDecoder objectDecoder( entity );
        JSON::Entity keyString, valueEntity;
        while ( objectDecoder.getNext( keyString, valueEntity ) )
        {
          try
          {
            HostSpec hostSpec = parseHostSpec( valueEntity );
            result.insert( HostStrings::value_type( keyString.stringToStdString(), hostSpec ) );
          }
          catch ( Exception e )
          {
            objectDecoder.rethrow( e );
          }
        }
      }
      else throw Exception( "must be a string or an object" );
    }

    static void parseHostStringsVector( JSON::Entity const &entity, HostStringsVector &result )
    {
      if ( entity.isArray() )
      {
        JSON::ArrayDecoder arrayDecoder( entity );
        JSON::Entity elementEntity;
        while ( !arrayDecoder.getNext( elementEntity ) )
        {
          try
          {
            HostStrings hostStrings;
            parseHostStrings( elementEntity, hostStrings );
            result.push_back( hostStrings );
          }
          catch ( Exception e )
          {
            arrayDecoder.rethrow( e );
          }
        }
      }
      else
      {
        HostStrings hostStrings;
        parseHostStrings( entity, hostStrings );
        result.push_back( hostStrings );
      }
    }
    
    void parseMethods( JSON::Entity const &methodsArray, std::vector<std::string> &methods )
    {
      JSON::ArrayDecoder methodArrayDecoder( methodsArray );
      JSON::Entity methodEntity;
      while ( methodArrayDecoder.getNext( methodEntity ) )
      {
        try
        {
          methodEntity.requireString();
          methods.push_back( methodEntity.stringToStdString() );
        }
        catch ( Exception e )
        {
          methodArrayDecoder.rethrow( e );
        }
      }
    }
    
    void parseInterface( JSON::Entity const &interfaceObject, Interface &interface )
    {
      JSON::ObjectDecoder interfaceObjectDecoder( interfaceObject );
      JSON::Entity keyString, valueEntity;
      while ( interfaceObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "methods", 7 ) )
          {
            valueEntity.requireArray();
            parseMethods( valueEntity, interface.methods );
          }
        }
        catch ( Exception e )
        {
          interfaceObjectDecoder.rethrow( e );
        }
      }
    }
    
    Desc parseDesc( JSON::Entity const &object )
    {
      Desc result;
      
      JSON::ObjectDecoder objectDecoder( object );
      JSON::Entity keyString, valueEntity;
      while ( objectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "libs", 4 ) )
          {
            parseHostStringsVector( valueEntity, result.libs );
          }
          else if ( keyString.stringIs( "code", 4 ) )
          {
            parseHostStringsVector( valueEntity, result.code );
          }
          else if ( keyString.stringIs( "jsConstants", 11 ) )
          {
            parseHostStringsVector( valueEntity, result.jsConstants );
          }
          else if ( keyString.stringIs( "interface", 9 ) )
          {
            valueEntity.requireObject();
            parseInterface( valueEntity, result.interface );
          }
        }
        catch ( Exception e )
        {
          objectDecoder.rethrow( e );
        }
      }
      
      return result;
    }
  };
};

