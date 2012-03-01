/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Base/Exception.h>

#if defined(FABRIC_POSIX)
# include <dirent.h>
# include <errno.h>
# include <fcntl.h>
# include <sys/stat.h>
# include <sys/types.h>
#elif defined(FABRIC_WIN32)
# include <windows.h>
#endif 

namespace Fabric
{
  namespace IO
  {
    RC::ConstHandle<Dir> Dir::Root()
    {
      static RC::Handle<Dir> root;
      if ( !root )
        root = new Dir( RC::ConstHandle<Dir>(), getRootPath(), true );
      return root;
    }

    RC::ConstHandle<Dir> Dir::User()
    {
      static RC::Handle<Dir> user;
      if ( !user )
        user = new Dir( Root(), "User", true );
      return user;
    }

    RC::ConstHandle<Dir> Dir::Private()
    {
      static RC::Handle<Dir> result;
      if ( !result )
        result = new Dir( Root(), ".private", true );
      return result;
    }

    Dir::Dir( RC::ConstHandle<Dir> const &parentDir, std::string const &entry, bool createIfMissing )
      : m_parentDir( parentDir )
      , m_entry( entry )
    {
      if ( m_parentDir )
        validateEntry( entry );
      
      if ( createIfMissing )
      {
        std::string fullPath = getFullPath();
        if ( !DirExists( fullPath ) )
	        CreateDir( fullPath );
      }
    }
    
    std::string Dir::getFullPath() const
    {
      if ( m_parentDir )
        return JoinPath( m_parentDir->getFullPath(), m_entry );
      else return m_entry;
    }

    std::vector<std::string> Dir::getFiles() const
    {
      std::vector<std::string> result;
#if defined(FABRIC_POSIX)
      DIR *dir = opendir( getFullPath().c_str() );
      if ( !dir )
        throw Exception("unable to open directory");
      for (;;)
      {
        struct dirent *de = readdir( dir );
        if ( !de )
          break;
        if ( de->d_type == DT_DIR )
          continue;
        result.push_back( std::string( de->d_name ) );
      }
      closedir( dir );
#elif defined(FABRIC_WIN32)
      WIN32_FIND_DATAA    fd;
      ::ZeroMemory( &fd, sizeof( fd ) );
      std::string   searchGlob = JoinPath( getFullPath(), "*" );
      HANDLE    hDir = ::FindFirstFileA( searchGlob.c_str(), &fd );
      if( hDir == INVALID_HANDLE_VALUE )
        throw Exception("unable to open directory");
      do
      {
        if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
          continue;
        result.push_back( std::string( fd.cFileName ) );
      } while( ::FindNextFileA( hDir, &fd ) );
      ::FindClose( hDir );
#endif 
      return result;
    }

    std::vector< RC::ConstHandle<Dir> > Dir::getSubDirs( bool followLinks ) const
    {
      std::string fullPath = getFullPath();
      std::vector<std::string> subDirEntries = GetSubDirEntries( fullPath, followLinks );
      
      std::vector< RC::ConstHandle<Dir> > result;
      for ( std::vector<std::string>::const_iterator it=subDirEntries.begin(); it!=subDirEntries.end(); ++it )
      {
        std::string const &subDirEntry = *it;
        result.push_back( Dir::Create( this, subDirEntry ) );
      }
      return result;
    }

    std::string Dir::getFileContents( std::string const &entry ) const
    {
      validateEntry( entry );
      std::string filePath = JoinPath( getFullPath(), entry );
      
#if defined(FABRIC_POSIX)
      int fd = open( filePath.c_str(), O_RDONLY );
      if ( fd < 0 )
        throw Exception("file not found");

      struct stat st;
      if ( fstat( fd, &st ) )
      {
        close( fd );
        throw Exception("unable to stat file");
      }
      if ( S_ISDIR( st.st_mode ) )
      {
        close( fd );
        throw Exception("not a file");
      }
      
      std::string result;
      for (;;)
      {
        char buf[65536];
        int count = read( fd, buf, 65536 );
        if ( count == -1 )
        {
          close( fd );
          throw Exception("error reading file");
        }
        if ( count == 0 )
          break;
        result.append( buf, count );
      }
      close( fd );
#elif defined(FABRIC_WIN32)
      HANDLE    hFile = ::CreateFileA( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );
      if( hFile == INVALID_HANDLE_VALUE )
        throw Exception( "Can't open file" );

      std::string result;
      for (;;)
      {
        char buf[65536];

        DWORD   nbRead = 0;
        if( !::ReadFile( hFile, buf, sizeof( buf ), &nbRead, NULL ) && ::GetLastError() != ERROR_HANDLE_EOF )
        {
          ::CloseHandle( hFile );
          throw Exception("error reading file");
        }

        if( nbRead )
          result.append( buf, nbRead );

        if( !nbRead || ::GetLastError() == ERROR_HANDLE_EOF )
          break;
      }

      ::CloseHandle( hFile );
#endif 
      return result;
    }

    void Dir::putFileContents( std::string const &entry, std::string const &contents ) const
    {
      validateEntry( entry );
      std::string filePath = JoinPath( getFullPath(), entry );
      
#if defined(FABRIC_POSIX)
      int fd = open( filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666 );
      if ( fd < 0 )
        throw Exception("unable to create file");

      int result = write( fd, contents.data(), contents.length() );
      close( fd );
      if ( size_t( result ) < contents.length() )
        throw Exception("short write");
#elif defined(FABRIC_WIN32)
      HANDLE    hFile = ::CreateFileA( filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
      if( hFile == INVALID_HANDLE_VALUE )
        throw Exception( "Can't create file" );

      DWORD   nbWritten = 0;
      BOOL    success = ::WriteFile( hFile, contents.data(), (DWORD)contents.length(), &nbWritten, 0 );
      ::CloseHandle( hFile );
      if( !success )
        throw Exception( "unable to write to file" );

      if( nbWritten < contents.length() )
        throw Exception( "short write" );
#endif 
    }

#if defined(FABRIC_WIN32)
    //[jcg 20110819] http://msdn.microsoft.com/en-us/library/ms724228(v=vs.85).aspx
    void TimetToWindowsFileTime( time_t t, LPFILETIME pft )
    {
        LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
        pft->dwLowDateTime = (DWORD) ll;
        pft->dwHighDateTime = ll >>32;
    }
#endif
      
    void Dir::recursiveDeleteFilesOlderThan( time_t time ) const
    {
      std::vector< RC::ConstHandle<Dir> > subDirs = getSubDirs( false );
      for ( std::vector< RC::ConstHandle<Dir> >::const_iterator it=subDirs.begin(); it!=subDirs.end(); ++it )
        (*it)->recursiveDeleteFilesOlderThan( time );
      
      std::string dirFullPath = getFullPath();
      std::vector<std::string> files = getFiles();
      for ( std::vector<std::string>::const_iterator it=files.begin(); it!=files.end(); ++it )
      {
        std::string const &file = *it;
        std::string fileFullPath = JoinPath( dirFullPath, file );

#if defined(FABRIC_POSIX)
        struct stat st;
        if ( lstat( fileFullPath.c_str(), &st ) != 0 )
        {
          FABRIC_LOG( "Warning: unable to return file information for " + _(fileFullPath) );
          continue;
        }
        if ( S_ISREG(st.st_mode) && st.st_ctime < time )
        {
          if ( unlink( fileFullPath.c_str() ) != 0 )
            FABRIC_LOG( "Warning: unable to delete " + _(fileFullPath) );
        }
#elif defined(FABRIC_WIN32)
        WIN32_FILE_ATTRIBUTE_DATA attrData;
        if( GetFileAttributesExA( fileFullPath.c_str(), GetFileExInfoStandard, &attrData) == 0 )
        {
          FABRIC_LOG( "Warning: unable to return file information for " + _(fileFullPath) );
          continue;
        }

        FILETIME timeTAsFileTime;
        TimetToWindowsFileTime(time, &timeTAsFileTime );

        if( CompareFileTime( &attrData.ftLastWriteTime, &timeTAsFileTime ) < 0 )
        {
          if ( !::DeleteFileA( fileFullPath.c_str() ) )
            FABRIC_LOG( "Warning: unable to delete " + _(fileFullPath) );
        }
#else
# error "unsupported FABRIC_PLATFORM_..."
#endif
      }
    }
  };
};
