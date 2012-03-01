/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "FileResourceProvider.h"
#include "Helpers.h"
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Base/Exception.h>
#include <string.h>
#include <Fabric/Core/MT/Impl.h>
#include <fstream>

namespace Fabric
{
  namespace IO
  {
    RC::Handle<FileResourceProvider> FileResourceProvider::Create( bool acceptUnformattedLocalPaths )
    {
      return new FileResourceProvider( acceptUnformattedLocalPaths );
    }

    FileResourceProvider::FileResourceProvider( bool acceptUnformattedLocalPaths )
      : m_acceptUnformattedLocalPaths( acceptUnformattedLocalPaths )
    {
    }

    char const* FileResourceProvider::getUrlScheme() const
    {
      return "file";
    }

    const size_t MaxFileReadBufferSize = 1<<19;//512K

    struct FileStream
    {
      std::string m_fullPath;
      std::ifstream file;
      size_t m_totalSize, m_nbRead;
      std::string m_mimeType;
      void* m_userData;
    };

    static void ReadFileJob( void *userdata, size_t index )
    {
      FileStream *fileStream = static_cast<FileStream *>( userdata );
      try
      {
        std::ifstream file( fileStream->m_fullPath.c_str(), std::ios::in | std::ios::binary );
        if( !file.is_open() )
          ResourceManager::onFailureAsyncThreadCall( "Unable to open file", fileStream->m_userData );//Don't put filename as it might be private
        else
        {
          std::vector<char> readBuffer;
          readBuffer.resize( std::min( MaxFileReadBufferSize, fileStream->m_totalSize ) );
          size_t nbRead = 0;
          while( nbRead < fileStream->m_totalSize )
          {
            size_t nbToRead = std::min( readBuffer.size(), fileStream->m_totalSize - nbRead );
            file.read( (char*)(&readBuffer.front()), nbToRead );
            if( file.bad() )
            {
              ResourceManager::onFailureAsyncThreadCall( "Error while reading file", fileStream->m_userData );//Don't put filename as it might be private
              break;
            }

            ResourceManager::onDataAsyncThreadCall( nbRead, nbToRead, &readBuffer.front(), fileStream->m_userData );
            nbRead += nbToRead;
            ResourceManager::onProgressAsyncThreadCall( fileStream->m_mimeType.c_str(), nbRead, fileStream->m_totalSize, fileStream->m_userData );
          }
        }
      }
      catch(...)
      {
        ResourceManager::onFailureAsyncThreadCall( "Unexpected error while reading file", fileStream->m_userData );//Don't put filename as it might be private
      }
    }

    static void FinishFileJob( void *userdata )
    {
      FileStream *fileStream = static_cast<FileStream *>( userdata );
      delete fileStream;
    }

    void FileResourceProvider::get( char const *url, bool getAsFile, void* userData )
    {
      if( strncmp( "file://", url, 7 ) == 0 )
      {
        url += 7;
      }
      else if( !m_acceptUnformattedLocalPaths || strstr( url, "//:" ) != NULL )
          throw Exception( "Error: URL not properly formatted for local files" );//Don't put filename as it might be private

      std::string fileWithPath = ChangeSeparatorsURLToFile( url );
      if( !FileExists( fileWithPath ) )
          throw Exception( "Error: file doesn't exist" );//Don't put filename as it might be private

      std::string extension = GetExtension( fileWithPath );
      size_t fileSize = GetFileSize( fileWithPath );

      if( getAsFile )
      {
        //To avoid client reentrancy problems, schedule a async callbacks
        ResourceManager::onFileAsyncThreadCall( fileWithPath.c_str(), userData );
        ResourceManager::onProgressAsyncThreadCall( extension.c_str(), fileSize, fileSize, userData );
      }
      else if( fileSize == 0 )
        ResourceManager::onProgressAsyncThreadCall( extension.c_str(), fileSize, fileSize, userData );
      else
      {
        FileStream *fileStream = new FileStream();
        fileStream->m_nbRead = 0;
        fileStream->m_totalSize = fileSize;
        fileStream->m_mimeType = extension;
        fileStream->m_fullPath = fileWithPath;
        fileStream->m_userData = userData;
        MT::ThreadPool::Instance()->executeParallelAsync( NULL, 1, ReadFileJob, fileStream, 0, FinishFileJob, fileStream );
      }
    }
  };
};
