/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Manager.h"
#include "Inst.h"

#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace Plug
  {
    RC::Handle<Manager> g_instance;

    RC::Handle<Manager> Manager::Instance()
    {
      if ( !g_instance )
        g_instance = new Manager();
      return g_instance;
    }

    void Manager::Terminate()
    {
      g_instance = 0;
    }

    Manager::Manager()
      : m_loaded( false )
      //, m_fabricSDKSOLibHandle( invalidSOLibHandle )
    {
#if 0
      try
      {
        m_fabricSDKSOLibHandle = SOLibOpen( "FabricSDK", m_fabricSDKSOLibResolvedName, true, m_pluginDirs );
      }
      catch ( Exception e )
      {
        FABRIC_LOG( "%s", ("Warning: Unable to load "+_(m_fabricSDKSOLibResolvedName)+", plugin support will not be available").c_str() );
      }
#endif      
    }
    
    void Manager::loadBuiltInPlugins( std::vector<std::string> const &pluginDirs, RC::Handle<CG::Manager> const &cgManager, EDK::Callbacks const &callbacks )
    {
      if ( !m_loaded )
      {
        //if ( m_fabricSDKSOLibHandle != invalidSOLibHandle )
        {
          for ( size_t i=0; i<pluginDirs.size(); ++i )
          {
            FABRIC_LOG( "Searching extension directory " + _(pluginDirs[i]) );
          
            RC::ConstHandle<IO::Dir> pluginsDir;
            std::vector<std::string> files;
            try
            {
              pluginsDir = IO::Dir::Create( 0, pluginDirs[i], false );
              files = pluginsDir->getFiles();
            }
            catch ( Exception e )
            {
              FABRIC_LOG( "Warning: unable to open extension directory " + _(pluginDirs[i]) );
              continue;
            }
              
            for ( size_t i=0; i<files.size(); ++i )
            {
              std::string const &filename = files[i];
              size_t length = filename.length();
              if ( length > 9 && filename.substr( length-9, 9 ) == ".fpm.json" )
              {
                std::string extensionName = filename.substr( 0, length-9 );
                std::string fpmContents = pluginsDir->getFileContents( filename );
                try
                {
                  registerPlugin( pluginsDir, extensionName, fpmContents, pluginDirs, cgManager, callbacks );
                }
                catch ( Exception e )
                {
                  FABRIC_LOG( "[%s] %s", extensionName.c_str(), (const char *)e );
                  FABRIC_LOG( "[%s] Error(s) encountered, extension disabled", extensionName.c_str() );
                  FABRIC_LOG( "[%s] Extension manifest is '%s'", extensionName.c_str(), IO::JoinPath( pluginsDir->getFullPath(), filename ).c_str() );
                }
              }
            }
          }
        }
        
        m_loaded = true;
      }

      CG::Diagnostics diagnostics;
      for ( NameToInstMap::const_iterator it=m_nameToInstMap.begin(); it!=m_nameToInstMap.end(); ++it )
      {
        it->second->getAST()->registerTypes( cgManager, diagnostics );
      }

      for ( std::map< std::string, void (*)( void * ) >::const_iterator it=m_implNameToDestructorMap.begin(); it!=m_implNameToDestructorMap.end(); ++it )
      {
        std::string const &implName = it->first;
        RC::ConstHandle<CG::Adapter> adapter = cgManager->getAdapter( implName );
        RC::ConstHandle<RT::Impl> impl = adapter->getImpl();
        void (*destructor)( void * ) = it->second;
        impl->setDisposeCallback( destructor );
      }
    }
    
    Manager::~Manager()
    {
      //if ( m_fabricSDKSOLibHandle )
      //  SOLibClose( m_fabricSDKSOLibHandle, m_fabricSDKSOLibResolvedName );
    }
    
    RC::ConstHandle<Inst> Manager::registerPlugin( RC::ConstHandle<IO::Dir> const &extensionDir, std::string const &name, std::string const &jsonDesc, std::vector<std::string> const &pluginDirs, RC::Handle<CG::Manager> const &cgManager, EDK::Callbacks const &callbacks )
    {
      RC::Handle<Inst> result;
      
      NameToInstMap::const_iterator it = m_nameToInstMap.find( name );
      if ( it != m_nameToInstMap.end() )
      {
        result = it->second;
        FABRIC_LOG( "[%s] Extension already registered", name.c_str() );
      }
      else
      {
        result = Inst::Create( extensionDir, name, jsonDesc, pluginDirs, cgManager, callbacks, m_implNameToDestructorMap );
        m_nameToInstMap.insert( NameToInstMap::value_type( name, result ) );
        FABRIC_LOG( "[%s] Extension registered", name.c_str() );
      }
      
      return result;
    }
      
    RC::ConstHandle<AST::GlobalList> Manager::getAST() const
    {
      RC::ConstHandle<AST::GlobalList> result;
      for ( NameToInstMap::const_iterator it=m_nameToInstMap.begin(); it!=m_nameToInstMap.end(); ++it )
        result = AST::GlobalList::Create( result, it->second->getAST() );
      return result;
    }
    
    void *Manager::llvmResolveExternalFunction( std::string const &name ) const
    {
      void *result = 0;
      for ( NameToInstMap::const_iterator it=m_nameToInstMap.begin(); it!=m_nameToInstMap.end(); ++it )
      {
        result = it->second->llvmResolveExternalFunction( name );
        if ( result )
          break;
      }
      return result;
    }

    void Manager::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( NameToInstMap::const_iterator it=m_nameToInstMap.begin(); it!=m_nameToInstMap.end(); ++it )
      {
        JSON::Encoder instEncoder = resultObjectEncoder.makeMember( it->first );
        it->second->jsonDesc( instEncoder );
      }
    }

    RC::ConstHandle<AST::GlobalList> Manager::maybeGetASTForExt( std::string const &extName ) const
    {
      RC::ConstHandle<AST::GlobalList> result;
      NameToInstMap::const_iterator it = m_nameToInstMap.find( extName );
      if ( it != m_nameToInstMap.end() )
        result = it->second->getAST();
      return result;
    }
  };
};
