/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Clients/NPAPI/ViewPort.h>
#include <Fabric/Clients/NPAPI/IOManager.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/DG/NamedObject.h>
#include <Fabric/Core/DG/CodeManager.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Dir.h>

namespace Fabric
{
  namespace NPAPI
  {
    static CG::CompileOptions const &GetCompileOptions()
    {
      static CG::CompileOptions compileOptions;
      static bool initialized = false;
      if ( !initialized )
      {
        initialized = true;
        compileOptions.setGuarded( true );
      }
      return compileOptions;
    }
    
    RC::Handle<Context> Context::Create(
      RC::Handle<IOManager> const &ioManager,
      std::vector<std::string> const &pluginDirs
      )
    {
      return new NPAPI::Context( ioManager, pluginDirs );
    }
    
    Context::Context(
      RC::Handle<IOManager> const &ioManager,
      std::vector<std::string> const &pluginDirs
      )
      : DG::Context( ioManager, pluginDirs, GetCompileOptions(), false )
      , m_ioManager( ioManager )
    {
    }
    
    Context::~Context()
    {
      FABRIC_ASSERT( m_viewPorts.empty() );
    }

    void Context::registerViewPort( std::string const &name, ViewPort *viewPort )
    {
      FABRIC_VERIFY( m_viewPorts.insert( ViewPorts::value_type( name, viewPort ) ).second );
    }
    
    void Context::unregisterViewPort( std::string const &name, ViewPort *viewPort )
    {
      ViewPorts::iterator it = m_viewPorts.find( name );
      FABRIC_ASSERT( it != m_viewPorts.end() );
      FABRIC_ASSERT( it->second == viewPort );
      m_viewPorts.erase( it );
    }

    RC::Handle<IOManager> Context::getIOManager() const
    {
      return m_ioManager;
    }

    void Context::jsonRoute(
      std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset >= 1 && dst[dstOffset].stringIs( "VP", 2 ) )
      {
        try
        {
          jsonRouteViewPorts( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
        }
        catch ( Exception e )
        {
          throw "'viewPorts': " + e;
        }
      }
      else return DG::Context::jsonRoute( dst, dstOffset, cmd, arg, resultArrayEncoder );
    }
    
    void Context::jsonExec(
      JSON::Entity const &cmd, JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      DG::Context::jsonExec( cmd, arg, resultArrayEncoder );
    }

    void Context::jsonRouteViewPorts( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd,
      JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( dst.size() - dstOffset == 1 )
      {
        std::string viewPortName = dst[dstOffset].stringToStdString();
        ViewPorts::const_iterator it = m_viewPorts.find( viewPortName );
        if ( it == m_viewPorts.end() )
          throw Exception( "unroutable" );
        
        it->second->jsonExec( cmd, arg, resultArrayEncoder );
      }
      else throw Exception( "unroutable" );
    }
    
    void Context::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      DG::Context::jsonDesc( resultObjectEncoder );
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "VP", 2 );
        jsonDescViewPorts( memberEncoder );
      }
    }
    
    void Context::jsonDescViewPorts( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( ViewPorts::const_iterator it=m_viewPorts.begin(); it!=m_viewPorts.end(); ++it )
      {
        JSON::Encoder viewPortEncoder = resultObjectEncoder.makeMember( it->first );
        it->second->jsonDesc( viewPortEncoder );
      }
    }

    std::string Context::queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension ) const
    {
      if( m_viewPorts.empty() )
        throw Exception( "Error: viewport is not initialized" );

      return m_viewPorts.begin()->second->queryUserFilePath( existingFile, title, defaultFilename, extension );
    }
  };
};
