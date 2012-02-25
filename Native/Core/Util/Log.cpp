/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Debug.h"

#if defined(FABRIC_OS_WINDOWS)

#include <stdint.h>
#include <fstream>

namespace Fabric
{
  namespace Util
  {
    class ErrorFile
    {
      std::ofstream m_file;

      ErrorFile( const char *filename );
      static ErrorFile *getInstance();
    public:
      static void LogCStr( char const *data, size_t length );
    };

    ErrorFile::ErrorFile( const char *filename )
      : m_file(filename, std::ios::out | std::ios::trunc )
    {
    }

    ErrorFile *ErrorFile::getInstance( )
    {
      static char fileName[1024];
      static int fileNameLen = snprintf(fileName, 1024, "%s\\FABRIC\\fabric-log.txt", getenv("APPDATA"));
      static ErrorFile errFile( fileName );
      return( &errFile );
    }

    void ErrorFile::LogCStr(char const *data, size_t length )
    {
      if( ::IsDebuggerPresent() )
      {
        ::OutputDebugStringA( data );
      }

      if( ::GetStdHandle( STD_OUTPUT_HANDLE ) != NULL )
        printf( data );

      if( getInstance()->m_file.is_open() )
      {
        getInstance()->m_file.write( data, length );
        getInstance()->m_file.flush();
      }
    }
  }
}

void FABRIC_WRITE_CSTR( char const *data, size_t length )
{
  Fabric::Util::ErrorFile::LogCStr( data, length );
}

#endif
