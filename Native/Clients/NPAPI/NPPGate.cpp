// Copyright 2010 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <Fabric/Clients/NPAPI/Interface.h>
#if defined(FABRIC_OS_MACOSX)
# include <Fabric/Clients/NPAPI/Darwin/WindowedCAViewPort.h>
# include <Fabric/Clients/NPAPI/Darwin/WindowedInvalidatingCAViewPort.h>
# if !defined(__x86_64__)
#  include <Fabric/Clients/NPAPI/Darwin/WindowlessCGViewPort.h>
# endif
#elif defined(FABRIC_OS_LINUX)
# include <Fabric/Clients/NPAPI/Linux/X11ViewPort.h>
#elif defined(FABRIC_OS_WINDOWS)
# include <Fabric/Clients/NPAPI/Windows/WindowsViewport.h>
#endif
#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Clients/NPAPI/IOManager.h>
#include <Fabric/Core/Build.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/Util/Assert.h>
#include <Fabric/Core/MT/IdleTaskQueue.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/OCL/OCL.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Core/Util/Debug.h>
#include <Fabric/Core/Build.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <npapi/npapi.h>
#include <npapi/npfunctions.h>
#include <new>
#if defined(FABRIC_POSIX)
# include <signal.h>
# include <execinfo.h>
#endif

class CALayer;

namespace Fabric
{
  namespace NPAPI
  {
#if defined(FABRIC_POSIX)
    static const size_t signalStackSize = 65536;
    static uint8_t signalStack[signalStackSize];

    static void fatalSignalTrap( int signum )
    {
      char const *msg1 = "Caught fatal signal ";
      write( 2, msg1, strlen(msg1) );
      char const *signame;
      switch ( signum )
      {
        case SIGILL: signame = "SIGILL"; break;
        case SIGTRAP: signame = "SIGTRAP"; break;
        case SIGABRT: signame = "SIGABRT"; break;
        case SIGBUS: signame = "SIGBUS"; break;
        case SIGFPE: signame = "SIGFPE"; break;
        case SIGSEGV: signame = "SIGSEGV"; break;
        default: signame = "<unknown>"; break;
      }
      write( 2, signame, strlen(signame) );
      char const *msg2 = "\n---- Backtrace:\n";
      write( 2, msg2, strlen(msg2) );

      static const int maxNumBacktracePtrs = 64;
      void *backtracePtrs[maxNumBacktracePtrs];
      int numBacktracePtrs = backtrace( backtracePtrs, maxNumBacktracePtrs );
      backtrace_symbols_fd( backtracePtrs, numBacktracePtrs, 2 );

      _exit( 0 );
    }
#endif

    NPError NPP_New(
      NPMIMEType mime_type,
      NPP npp,
      uint16_t mode,
      int16_t argc,
      char *argn[],
      char *argv[],
      NPSavedData *saved
      )
    {
      if ( !npp )
        return NPERR_INVALID_INSTANCE_ERROR;

#if defined(FABRIC_POSIX)
      stack_t stack;
      stack.ss_sp = &signalStack[0];
      stack.ss_flags = 0;
      stack.ss_size = signalStackSize;
      sigaltstack( &stack, NULL );

      struct sigaction sa;
      sa.sa_handler = &fatalSignalTrap;
      sigemptyset( &sa.sa_mask );
      sa.sa_flags = SA_ONSTACK;
      sigaction( SIGILL, &sa, NULL );
      sigaction( SIGTRAP, &sa, NULL );
      sigaction( SIGABRT, &sa, NULL );
      sigaction( SIGBUS, &sa, NULL );
      sigaction( SIGFPE, &sa, NULL );
      sigaction( SIGSEGV, &sa, NULL );
#endif

      std::string contextID;
      enum
      {
        VPT_EMPTY,
        VPT_3D
      } viewPortType = VPT_EMPTY;
      bool compositing = false;
      for ( int16_t i=0; i<argc; ++i )
      {
        if ( strcmp( argn[i], "windowType" ) == 0 )
        {
          if ( strcmp( argv[i], "3d" ) == 0 )
            viewPortType = VPT_3D;
          else if ( strcmp( argv[i], "empty" ) == 0 )
            viewPortType = VPT_EMPTY;
        }
        else if ( strcmp( argn[i], "contextID" ) == 0 )
          contextID = argv[i];
        else if ( strcmp( argn[i], "compositing" ) == 0 )
          compositing = true;
      }
      
      RC::Handle<Context> context;
      if ( contextID.length() > 0 )
      {
        context = RC::Handle<Context>::StaticCast( DG::Context::Bind( contextID ) );
        FABRIC_DEBUG_LOG( "Bound to existing context '%s'", contextID.c_str() );
      }
      else
      {
      	std::vector<std::string> pluginPaths;

        std::string googleChromeProfilesPath;
        std::string chromiumProfilesPath;
        std::string firefoxProfilesPath;

#if defined(FABRIC_OS_MACOSX)
        char const *home = getenv("HOME");
        if ( home && *home )
        {
          std::string homePath( home );
          std::string libraryPath = IO::JoinPath( homePath, "Library" );

          pluginPaths.push_back( IO::JoinPath( libraryPath, "Fabric", "Exts" ) );

          std::string applicationSupportPath = IO::JoinPath( libraryPath, "Application Support" );
          googleChromeProfilesPath = IO::JoinPath( applicationSupportPath, "Google", "Chrome" );
          chromiumProfilesPath = IO::JoinPath( applicationSupportPath, "Chromium" );
          firefoxProfilesPath = IO::JoinPath( applicationSupportPath, "Firefox", "Profiles" );
        }
        pluginPaths.push_back( "/Library/Fabric/Exts" );
#elif defined(FABRIC_OS_LINUX)
        char const *home = getenv("HOME");
        if ( home && *home )
        {
          std::string homePath( home );
          pluginPaths.push_back( IO::JoinPath( homePath, ".fabric", "Exts" ) );
          googleChromeProfilesPath = IO::JoinPath( homePath, ".config", "google-chrome" );
          chromiumProfilesPath = IO::JoinPath( homePath, ".config", "chromium" );
          firefoxProfilesPath = IO::JoinPath( homePath, ".mozilla", "firefox" );
        }
        pluginPaths.push_back( "/usr/lib/fabric/Exts" );
#elif defined(FABRIC_OS_WINDOWS)
        char const *appData = getenv("APPDATA");
        if ( appData && *appData )
        {
          std::string appDataDir(appData);
          pluginPaths.push_back( IO::JoinPath( appDataDir, "Fabric" , "Exts" ) );

          
          firefoxProfilesPath = IO::JoinPath( appDataDir, "Mozilla", "Firefox", "Profiles" );
        }

        char const *localAppData = getenv("LOCALAPPDATA");
        if ( localAppData && *localAppData )
        {
          std::string localAppDataPath(localAppData);
          googleChromeProfilesPath = IO::JoinPath( localAppDataPath, "Google", "Chrome", "User Data" );
          chromiumProfilesPath = IO::JoinPath( localAppDataPath, "Chromium", "User Data" );
        }
#endif

        std::string chromeExtensionsPathSpec = IO::JoinPath( "Default", "Extensions", "kdijpapodgbchkehlmacojcegohcmbel", std::string(buildPureVersion) + "_*" );
        IO::GlobDirPaths( IO::JoinPath( googleChromeProfilesPath, chromeExtensionsPathSpec ), pluginPaths );
        IO::GlobDirPaths( IO::JoinPath( chromiumProfilesPath, chromeExtensionsPathSpec ), pluginPaths );

        std::string firefoxExtensionsPathSpec = IO::JoinPath( "*", "extensions", "info@fabric-engine.com", "plugins" );
        IO::GlobDirPaths( IO::JoinPath( firefoxProfilesPath, firefoxExtensionsPathSpec ), pluginPaths );
      
        RC::Handle<IOManager> ioManager = IOManager::Create( npp );
        context = Context::Create( ioManager, pluginPaths );
        Plug::Manager::Instance()->loadBuiltInPlugins( pluginPaths, context->getCGManager() );
        
        contextID = context->getContextID();
        FABRIC_DEBUG_LOG( "Created new context '%s'", contextID.c_str() );
    
        OCL::registerTypes( context->getRTManager() );
      }
      
      RC::Handle<Interface> interface = Interface::Create( npp, context );

      RC::Handle<ViewPort> viewPort;
      if ( viewPortType != VPT_EMPTY )
      {
#if defined(FABRIC_OS_NACL)
        viewPort = NPNaClViewPort::Create();
#elif defined(FABRIC_OS_MACOSX)
        if ( compositing )
        {
# if !defined(__x86_64__)
          viewPort = WindowlessCGViewPort::Create( interface );
# else
          FABRIC_LOG( "Compositing viewport not supported in 64-bit mode" );
          return NPERR_GENERIC_ERROR;
# endif
        }
        else
        {
          viewPort = WindowedInvalidatingCAViewPort::Create( interface );
          if ( !viewPort )
            viewPort = WindowedCAViewPort::Create( interface );
        }
#elif defined(FABRIC_OS_LINUX)
        viewPort = X11ViewPort::Create( interface );
#elif defined(FABRIC_OS_WINDOWS)
        viewPort = WindowsViewPort::Create( interface );
#endif
        if ( !viewPort )
        {
          FABRIC_LOG( "Unable to create a viewport for your browser!" );
          return NPERR_GENERIC_ERROR;
        }
        else
        {
          interface->setViewPort( viewPort );
      
          std::vector<std::string> src;
          src.push_back("VP");
          src.push_back("viewPort");
          
          context->jsonNotify( src, "delta", viewPort->jsonDesc() ); 
        }
      }
      
      npp->pdata = interface.take();

      return NPERR_NO_ERROR;
    }

    NPError NPP_Destroy( NPP npp, NPSavedData** save )
    {
      if ( !npp )
        return NPERR_INVALID_INSTANCE_ERROR;
        
      MT::IdleTaskQueue::Instance()->wait();
      
      Interface *interface = static_cast<Interface *>( npp->pdata );
      // [pzion 20110228] Safari sometimes calls NPP_Destroy with an uninitialized instance
      if ( interface )
      {
        NPError npError = interface->nppDestroy( npp, save );
        interface->release();
        return npError;
      }
      else return NPERR_NO_ERROR;
    }

    NPError NPP_GetValue( NPP npp, NPPVariable variable, void *value )
    {
      switch ( variable )
      {
        case NPPVpluginNameString:
          *static_cast<char const **>( value ) = buildName;
          return NPERR_NO_ERROR;
        case NPPVpluginDescriptionString:
          *static_cast<char const **>( value ) = buildDesc;
          return NPERR_NO_ERROR;
#if defined(FABRIC_OS_LINUX)
        // [pzion 20110211] Special case: Linux plugin requires XEmbed no matter what
        case NPPVpluginNeedsXEmbed:
          *(static_cast<NPBool*>(value)) = true;
          return NPERR_NO_ERROR;
#endif
        default:      
          if ( !npp )
            return NPERR_INVALID_INSTANCE_ERROR;
          return static_cast<Interface *>( npp->pdata )->nppGetValue( npp, variable, value );
      }
    }

    // |event| just took place in this plugin's window in the browser.  This
    // function should return true if the event was handled, false if it was
    // ignored.
    // Declaration: npapi.h
    // Documentation URL: https://developer.mozilla.org/en/NPP_HandleEvent
    int16_t NPP_HandleEvent( NPP npp, void* event )
    {
      if ( !npp )
        return NPERR_INVALID_INSTANCE_ERROR;
      Interface *interface = static_cast<Interface *>( npp->pdata );
      
      return interface->nppHandleEvent( npp, event );
    }

    NPError NPP_SetWindow( NPP npp, NPWindow *window )
    {
      if ( !npp )
        return NPERR_INVALID_INSTANCE_ERROR;
      Interface *interface = static_cast<Interface *>( npp->pdata );
      return interface->nppSetWindow( npp, window );
    }

    NPError NPP_NewStream( NPP npp, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype )
    {
      if( !npp )
        return NPERR_INVALID_INSTANCE_ERROR;
      Interface *interface = static_cast<Interface *>( npp->pdata );
      NPError result = NPERR_NO_ERROR;
      try
      {
        result = interface->nppNewStream( npp, type, stream, seekable, stype );
      }
      catch ( Fabric::Exception e )
      {
        FABRIC_DEBUG_LOG( "NPP_NewStream: caught Fabric exception: " + e );
      }
      catch ( ... )
      {
        FABRIC_DEBUG_LOG( "NPP_NewStream: caught unknown exception" );
      }
      return result;
    }

    int32_t NPP_WriteReady( NPP npp, NPStream* stream )
    {
      FABRIC_ASSERT( npp );
      Interface *interface = static_cast<Interface *>( npp->pdata );
      return interface->nppWriteReady( npp, stream );
    }

    int32_t NPP_Write( NPP npp, NPStream* stream, int32_t offset, int32_t len, void* buffer )
    {
      FABRIC_ASSERT( npp );
      Interface *interface = static_cast<Interface *>( npp->pdata );
      int32_t result = 0;
      try
      {
        result = interface->nppWrite( npp, stream, offset, len, buffer );
      }
      catch ( Fabric::Exception e )
      {
        FABRIC_DEBUG_LOG( "NPP_Write: caught Fabric exception: " + e );
      }
      catch ( ... )
      {
        FABRIC_DEBUG_LOG( "NPP_Write: caught unknown exception" );
      }
      return result;
    }

    NPError NPP_DestroyStream( NPP npp, NPStream *stream, NPReason reason )
    {
      if( !npp )
        return NPERR_INVALID_INSTANCE_ERROR;
      Interface *interface = static_cast<Interface *>( npp->pdata );
      NPError result = NPERR_NO_ERROR;
      try
      {
        result = interface->nppDestroyStream( npp, stream, reason );
      }
      catch ( Fabric::Exception e )
      {
        FABRIC_DEBUG_LOG( "NPP_DestroyStream: caught Fabric exception: " + e );
      }
      catch ( ... )
      {
        FABRIC_DEBUG_LOG( "NPP_DestroyStream: caught unknown exception" );
      }
      return result;
    }
    
    void NPP_URLNotify(NPP npp, const char* url, NPReason reason, void* notifyData)
    {
      // It's just here for Safari.
    }
  };
};

extern "C" NPError InitializePluginFunctions( NPPluginFuncs *npPluginFuncs )
{
  memset( npPluginFuncs, 0, sizeof(*npPluginFuncs) );
  npPluginFuncs->version = NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL;
  npPluginFuncs->size = sizeof(*npPluginFuncs);
  
  npPluginFuncs->newp = &Fabric::NPAPI::NPP_New;
  npPluginFuncs->destroy = &Fabric::NPAPI::NPP_Destroy;
  npPluginFuncs->setwindow = &Fabric::NPAPI::NPP_SetWindow;
  npPluginFuncs->event = &Fabric::NPAPI::NPP_HandleEvent;
  npPluginFuncs->getvalue = &Fabric::NPAPI::NPP_GetValue;
  npPluginFuncs->newstream = &Fabric::NPAPI::NPP_NewStream;
  npPluginFuncs->writeready = &Fabric::NPAPI::NPP_WriteReady;
  npPluginFuncs->write = &Fabric::NPAPI::NPP_Write;
  npPluginFuncs->destroystream = &Fabric::NPAPI::NPP_DestroyStream;
  npPluginFuncs->urlnotify = &Fabric::NPAPI::NPP_URLNotify;
  
  return NPERR_NO_ERROR;
}
