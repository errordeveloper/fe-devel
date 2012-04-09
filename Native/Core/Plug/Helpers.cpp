/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/Plug/Helpers.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Base/Util/Log.h>

#if defined(FABRIC_OS_MACOSX) || defined(FABRIC_OS_LINUX)
# include <dlfcn.h>
#elif defined(FABRIC_OS_WINDOWS)
# include <windows.h>
#else
# error "Unsupported platform"
#endif

namespace Fabric
{
  namespace Plug
  {
    extern SOLibHandle const invalidSOLibHandle = 0;

    SOLibHandle SOLibOpen( std::string const &name, std::string &resolvedName, bool global, std::vector<std::string> const &pluginDirs )
    {
#if defined(FABRIC_OS_MACOSX)
      resolvedName = "lib" + name + ".dylib";
#elif defined(FABRIC_OS_LINUX)
      resolvedName = "lib" + name + ".so";
#elif defined(FABRIC_OS_WINDOWS)
      resolvedName = name + ".dll";
#else
# error "Unsupported platform"
#endif

      SOLibHandle result = invalidSOLibHandle;
      
      // [pzion 20110418] First, check for the plugin explcitly in the given directories
      for ( size_t i=0; i<pluginDirs.size(); ++i )
      {
#if defined(FABRIC_POSIX)
        result = dlopen( IO::JoinPath( pluginDirs[i], resolvedName ).c_str(), RTLD_LAZY | (global?RTLD_GLOBAL:RTLD_LOCAL) );
#elif defined(FABRIC_WIN32)
        result = ::LoadLibraryExA( IO::JoinPath( pluginDirs[i], resolvedName ).c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
#else
# error "Unsupported platform"
#endif
        if ( result != invalidSOLibHandle )
          break;
      }
      
      // [pzion 20110418] Next, check on the general system path
      if ( result == invalidSOLibHandle )
      {
#if defined(FABRIC_POSIX)
        result = dlopen( resolvedName.c_str(), RTLD_LAZY | (global?RTLD_GLOBAL:RTLD_LOCAL) );
#elif defined(FABRIC_WIN32)
        result = ::LoadLibraryExA( resolvedName.c_str(), NULL, 0 );
#else
# error "Unsupported platform"
#endif
        if ( result == invalidSOLibHandle )
        {
          std::string msg;
          
#if defined(FABRIC_POSIX)
          msg = dlerror();
#elif defined(FABRIC_WIN32)
          char *szMessage = 0;
          ::FormatMessageA( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, ::GetLastError(), 0, (LPTSTR)&szMessage, 0, 0 );
          msg = std::string( szMessage ? szMessage : "" );
          ::LocalFree( szMessage );
#else
# error "Unsupported platform"
#endif
          throw Exception( "unable to open " + _(resolvedName) + ": " + msg );
        }
      }

      return result;
    }
    
    void *SOLibResolve( SOLibHandle soLibHandle, std::string const &functionName )
    {
#if defined(FABRIC_OS_MACOSX) || defined(FABRIC_OS_LINUX)
      return dlsym( soLibHandle, functionName.c_str() );
#elif defined(FABRIC_OS_WINDOWS)
      return ::GetProcAddress( soLibHandle, functionName.c_str() );
#else
# error "Unsupported platform"
#endif
    }
    
    void SOLibClose( SOLibHandle soLibHandle, std::string const &resolvedName )
    {
      FABRIC_ASSERT( soLibHandle != invalidSOLibHandle );
#if defined(FABRIC_OS_MACOSX) || defined(FABRIC_OS_LINUX)
      int result = dlclose( soLibHandle );
      if ( result != 0 )
        FABRIC_LOG( "Warning: unable to close dynamic library '%s'", resolvedName.c_str() );
#elif defined(FABRIC_OS_WINDOWS)
      if( !::FreeLibrary( soLibHandle ) )
        FABRIC_LOG( "Warning: unable to close dynamic library '%s'", resolvedName.c_str() );
#else
# error "Unsupported platform"
#endif
    }

    void AppendUserPaths( std::vector<std::string> &paths )
    {
      char const *fabricExtPath = getenv("FABRIC_EXT_PATH");
      if ( fabricExtPath && *fabricExtPath )
      {
        std::string fabricExtPathString( fabricExtPath );
        size_t findPos = 0;
        while ( findPos < fabricExtPathString.size() )
        {
#if defined(FABRIC_WIN32)
          static const char sep = ';';
#elif defined(FABRIC_POSIX)
          static const char sep = ':';
#else
 #error "Unsupported platform"
#endif
          size_t pos = fabricExtPathString.find( sep, findPos );
          if ( pos == std::string::npos )
            pos = fabricExtPathString.size();
          if ( pos - findPos > 0 )
            paths.push_back( fabricExtPathString.substr( findPos, pos - findPos ) );
          findPos = pos + 1;
        }
      }

#if defined(FABRIC_OS_MACOSX)
      char const *home = getenv("HOME");
      if ( home && *home )
      {
        std::string homePath( home );
        paths.push_back( IO::JoinPath( homePath, "Library", "Fabric", "Exts" ) );
      }
#elif defined(FABRIC_OS_LINUX)
      char const *home = getenv("HOME");
      if ( home && *home )
      {
        std::string homePath( home );
        paths.push_back( IO::JoinPath( homePath, ".fabric", "Exts" ) );
      }
#elif defined(FABRIC_OS_WINDOWS)
      // [pzion 20120409] No user path defined for Windows??
#endif
    }

    void AppendGlobalPaths( std::vector<std::string> &paths )
    {
#if defined(FABRIC_OS_MACOSX)
      paths.push_back( "/Library/Fabric/Exts" );
#elif defined(FABRIC_OS_LINUX)
      paths.push_back( "/usr/lib/fabric/Exts" );
#elif defined(FABRIC_OS_WINDOWS)
      char const *appData = getenv("APPDATA");
      if ( appData && *appData )
      {
        std::string appDataDir(appData);
        paths.push_back( IO::JoinPath( appDataDir, "Fabric" , "Exts" ) );
      }
#endif
    }
  }
}
