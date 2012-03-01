/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_HELPERS_H
#define _FABRIC_IO_HELPERS_H

#include <string>
#include <vector>

namespace Fabric
{
  namespace IO
  {
    void validateEntry( std::string const &entry );
    void validateAbsolutePath( std::string const &entry );
    
    std::string const &getRootPath();
    
    std::string JoinPath( std::string const &lhs, std::string const &rhs );
    inline std::string JoinPath( std::string const &arg1, std::string const &arg2, std::string const &arg3 )
    {
      return JoinPath( JoinPath( arg1, arg2 ), arg3 );
    }
    inline std::string JoinPath( std::string const &arg1, std::string const &arg2, std::string const &arg3, std::string const &arg4 )
    {
      return JoinPath( JoinPath( arg1, arg2, arg3 ), arg4 );
    }
    inline std::string JoinPath( std::string const &arg1, std::string const &arg2, std::string const &arg3, std::string const &arg4, std::string const &arg5 )
    {
      return JoinPath( JoinPath( arg1, arg2, arg3, arg4 ), arg5 );
    }
    inline std::string JoinPath( std::string const &arg1, std::string const &arg2, std::string const &arg3, std::string const &arg4, std::string const &arg5, std::string const &arg6 )
    {
      return JoinPath( JoinPath( arg1, arg2, arg3, arg4, arg5 ), arg6 );
    }
    inline std::string JoinPath( std::string const &arg1, std::string const &arg2, std::string const &arg3, std::string const &arg4, std::string const &arg5, std::string const &arg6, std::string const &arg7 )
    {
      return JoinPath( JoinPath( arg1, arg2, arg3, arg4, arg5, arg6 ), arg7 );
    }

    void SplitPath( std::string const &path, std::string &parentDir, std::string &entry );
    std::string GetExtension( std::string const &filename );
    std::string GetURLExtension( std::string const &url );

    std::string ChangeSeparatorsURLToFile( std::string const &url );
    std::string ChangeSeparatorsFileToURL( std::string const &filePath );

    //void safeCall( void (*callback)( int fd ) );
    
    bool DirExists( std::string const &dirPath );
    bool FileExists( std::string const &fullPath );
    size_t GetFileSize( std::string const &fullPath );
    bool IsLink( std::string const &fullPath );
    void CreateDir( std::string const &dirPath );
    std::vector<std::string> GetSubDirEntries( std::string const &dirPath, bool followLinks = true );
    void CopyFile_( std::string const &sourceFullPath, std::string const &targetFullPath );
    
    void GlobDirPaths( std::string const &dirPathSpec, std::vector<std::string> &result );

  };
};

#endif //_FABRIC_IO_HELPERS_H
