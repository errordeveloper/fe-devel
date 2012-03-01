/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/Util/Triple.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Config.h>

#include <ctype.h>

namespace Fabric
{
  namespace Util
  {
    std::string const &getHostTriple()
    {
      static std::string hostTriple;
      if ( hostTriple.empty() )
      {
#if defined( FABRIC_OS_MACOSX )
        if ( sizeof(size_t) == 8 )
          hostTriple += "x86_64-";
        else
          hostTriple += "i386-";
        hostTriple += "osx-";
        FILE *fp = popen( "/usr/bin/defaults read /System/Library/CoreServices/SystemVersion ProductVersion", "r" );
        while ( !feof(fp) )
        {
          char buf[256];
          size_t read = fread( buf, 1, 256, fp );
          while ( read > 0 && isspace( buf[read-1] ) ) --read;
          hostTriple += std::string( buf, read );
        }
        pclose( fp );
#elif defined( FABRIC_OS_LINUX )
        hostTriple = sizeof(size_t)==8? "x86_64-linux-unknown": "i386-linux-unknown";
#elif defined( FABRIC_OS_WINDOWS )
        hostTriple = sizeof(size_t)==8? "x86_64-win-xp": "i386-win-xp";
#else
# error "Unsupported platform"
#endif
      }
      return hostTriple;
    }

    bool matchHostTriple( std::string const &hostTriple, std::string const &pattern )
    {
      if ( pattern == "*" )
        return true;

      char const *h = hostTriple.data();
      char const *hEnd = h + hostTriple.length();
      
      char const *p = pattern.data();
      char const *pEnd = p + pattern.length();
      
      while ( h != hEnd && p != pEnd )
      {
        if ( *p == '*' )
        {
          if ( ++p != pEnd && *p != '-' )
            throw Exception( "malformed pattern " + _(pattern) );
          while ( h != hEnd && *h != '-' )
            ++h;
        }
        else
        {
          if ( *p++ != *h++ )
            return false;
        }
      }
      
      return h == hEnd && p == pEnd;
    }
  };
};
