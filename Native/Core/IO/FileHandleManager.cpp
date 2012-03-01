/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "FileHandleManager.h"
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/Util/Base64.h>
#include <Fabric/Core/Util/Random.h>
#include <string.h>
#include <fstream>

namespace Fabric
{
  namespace IO
  {
    FileHandleManager::FileHandleManager()
    {
    }

    bool FileHandleManager::targetExists( std::string const &handle ) const
    {
      std::string path = getPath( handle );
      if( hasRelativePath( handle ) )//'isfolder' is unknown
        return DirExists( path ) || FileExists( path );

      if( isFolder( handle ) )
        return DirExists( path );
      else
        return FileExists( path );
    }

    void FileHandleManager::ensureTargetExists( std::string const &handle ) const
    {
      if( !targetExists( handle ) )
      {
        if( isReadOnly( handle ) )
          throw Exception( "Error: cannot create file or folders flagged as read-only" );

        if( hasRelativePath( handle ) )
          throw Exception( "Error: ambiguous: handle's relative path could be a file or a folder; first create a file or folder handle from relative path" );

        std::string path = getPath( handle );
        if( isFolder( handle ) )
          CreateDir( path );
        else
        {
          std::string dir, file;
          SplitPath( path, dir, file );
          if( !DirExists( dir ) )
            CreateDir( dir );
          if( !FileExists( path ) )
          {
            std::ofstream file( path.c_str() );
            if( !file.is_open() )
              throw Exception( "Unable to create file" );
          }
        }
      }
    }

    void FileHandleManager::putFile( std::string const &handle, size_t size, const void* byteData, bool append ) const
    {
      if( isReadOnly( handle ) )
        throw Exception( "File has no write access" );

      if( isFolder( handle ) )
        throw Exception( "Handle is a folder; must be a file" );

      ensureTargetExists( handle );
      std::string path = getPath( handle );

      std::ofstream file( path.c_str(), std::ios::out | (append ? std::ios::app : std::ios::trunc) | std::ios::binary );
      if( !file.is_open() )
        throw Exception( "Unable to create file" );

      file.write( (const char*)byteData, size );
      if( file.bad() )
        throw Exception( "Error while writing to file" );
    }

    void FileHandleManager::copyFile( std::string const &source, std::string const &target ) const
    {
      if( target == source)
        return;

      if( isReadOnly( target ) )
        throw Exception( "Target file has no write access" );

      if( isFolder( target ) )
        throw Exception( "Target handle is a folder; must be a file" );

      std::string targetPath = getPath( target );

      if( !targetExists( source ) )
        throw Exception( "Source file not found" );

      if( isFolder( source ) )
        throw Exception( "Source handle is a folder; must be a file" );

      std::string sourcePath = getPath( source );
      IO::CopyFile_( sourcePath, targetPath );
    }
  };
};
