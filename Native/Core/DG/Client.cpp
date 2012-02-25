/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/Client.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/CompiledObject.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace DG
  {
    RC::Handle<Context> Client::getContext() const
    {
      return m_context;
    }
    
    Client::Client( RC::Handle<Context> const &context )
      : m_context( context )
    {
      m_context->registerClient( this );
    }
    
    Client::~Client()
    {
      m_context->unregisterClient( this );
    }

    void Client::jsonExec(
      char const *jsonEncodedCommandsData,
      size_t jsonEncodedCommandsLength,
      JSON::Encoder &resultEncoder
      ) const
    {
      Context::NotificationBracket notificationBracket( m_context );
      
      JSON::Decoder decoder( jsonEncodedCommandsData, jsonEncodedCommandsLength );
      JSON::Entity entity;
      if ( !decoder.getNext( entity ) )
        throw "missing JSON entity";
      
      JSON::ArrayEncoder resultArrayEncoder = resultEncoder.makeArray();

      entity.requireArray();
      JSON::ArrayDecoder arrayDecoder( entity );
      JSON::Entity elementEntity;
      size_t cmdIndex = 0;
      while ( arrayDecoder.getNext( elementEntity ) )
      {
        std::vector<JSON::Entity> dst;
        JSON::Entity cmd;
        JSON::Entity arg;
        
        try
        {
          elementEntity.requireObject();
          JSON::ObjectDecoder objectDecoder( elementEntity );
          JSON::Entity keyString, valueEntity;
          while ( objectDecoder.getNext( keyString, valueEntity ) )
          {
            try
            {
              if ( keyString.stringIs( "cmd", 3 ) )
              {
                valueEntity.requireString();
                cmd = valueEntity;
              }
              else if ( keyString.stringIs( "dst", 3 ) )
              {
                valueEntity.requireArray();
                JSON::ArrayDecoder dstArrayDecoder( valueEntity );
                JSON::Entity dstElementEntity;
                while ( dstArrayDecoder.getNext( dstElementEntity ) )
                {
                  try
                  {
                    dstElementEntity.requireString();
                    dst.push_back( dstElementEntity );
                  }
                  catch ( Exception e )
                  {
                    dstArrayDecoder.rethrow( e );
                  }
                }
              }
              else if ( keyString.stringIs( "arg", 3 ) )
                arg = valueEntity;
              else throw Exception( "unexpected" );
            }
            catch ( Exception e )
            {
              objectDecoder.rethrow( e );
            }
          }
          
          if ( !cmd )
            throw Exception( "missing 'cmd'" );
        }
        catch ( Exception e )
        {
          throw "command index " + _(cmdIndex) + ": " + e;
        }

        JSON::ObjectEncoder resultObjectEncoder = resultArrayEncoder.makeElement().makeObject();
        try
        {
          try
          {
            Util::SimpleString commandResult;
            JSON::Encoder commandResultEncoder( &commandResult );
            JSON::ArrayEncoder commandResultArrayEncoder = commandResultEncoder.makeArray();
            m_context->jsonRoute( dst, 0, cmd, arg, commandResultArrayEncoder );
            if ( commandResultArrayEncoder.getCount() > 0 )
            {
              commandResultArrayEncoder.flush();
              commandResultEncoder.flush();
              JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "result", 6 );
              memberEncoder.appendJSON( commandResult.data()+1, commandResult.length()-2 );
            }
          }
          catch ( Exception e )
          {
            std::string prefix;
            for ( std::vector<JSON::Entity>::const_iterator it=dst.begin(); it!=dst.end(); ++it )
            {
              prefix += it->stringToStdString();
              prefix += '.';
            }
            
            prefix += cmd.stringToStdString();
            prefix += '(';
            if ( arg )
              prefix += _( arg.data, arg.length, 32 );
            prefix += "): ";
            
            throw prefix + e;
          }
        }
        catch ( Exception e )
        {
          resultObjectEncoder.makeMember( "exception", 9 ).makeString( e.getData(), e.getLength() );
          break;
        }
      }
      
      if ( decoder.getNext( entity ) )
        throw "extra JSON entity";
      
      CompiledObject::PrepareForExecution( m_context );
    }
    
    void Client::notifyInitialState() const
    {
      Util::SimpleString json;
      {
        JSON::Encoder encoder( &json );
        JSON::ArrayEncoder arrayEncoder = encoder.makeArray();
        
        {
          JSON::Encoder encoder = arrayEncoder.makeElement();
          JSON::ObjectEncoder objectEncoder = encoder.makeObject();
          
          {
            JSON::Encoder srcEncoder = objectEncoder.makeMember( "src", 3 );
            JSON::ArrayEncoder srcArrayEncoder = srcEncoder.makeArray();
          }
          
          {
            JSON::Encoder cmdEncoder = objectEncoder.makeMember( "cmd", 3 );
            cmdEncoder.makeString( "state", 5 );
          }
          
          {
            JSON::Encoder argEncoder = objectEncoder.makeMember( "arg", 3 );
            jsonDesc( argEncoder );
          }
        }
      }
      notify( json );
    }

    void Client::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 1 && dst[dstOffset].stringIs( "client", 6 ) )
      {
        try
        {
          jsonExec( cmd, arg, resultArrayEncoder );
        }
        catch ( Exception e )
        {
          throw "'client': command " + _(cmd.stringToStdString()) + ": " + e;
        }
      }
      else m_context->jsonRoute( dst, dstOffset, cmd, arg, resultArrayEncoder );
    }
    
    void Client::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      throw Exception( "unknown command" );
    }
    
    void Client::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      return m_context->jsonDesc( resultEncoder );
    }
  };
};
