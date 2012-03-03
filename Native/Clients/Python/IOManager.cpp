/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/Python/IOManager.h>
#include <Fabric/Core/IO/SimpleFileHandleManager.h>
#include <Fabric/Core/IO/FileResourceProvider.h>

#include <fstream>
#include <string>

namespace Fabric
{
  namespace Python
  {
    RC::Handle<TestSynchronousFileResourceProvider>
        TestSynchronousFileResourceProvider::Create()
    {
      return new TestSynchronousFileResourceProvider();
    }

    char const *TestSynchronousFileResourceProvider::getUrlScheme() const
    {
      return "testfile";
    }

    void TestSynchronousFileResourceProvider::get(
        char const *url,
        bool getAsFile,
        void* userData
        )
    {
      if( strncmp( "testfile://", url, 11 ) != 0 )
          throw Exception( "Error: URL not properly formatted for local files" );//Don't put filename as it might be private

      std::string fileWithPath = IO::ChangeSeparatorsURLToFile( std::string( url+11 ) );
      if( !IO::FileExists( fileWithPath ) )
          throw Exception( "Error: file doesn't exist" );//Don't put filename as it might be private

      std::string extension = IO::GetExtension( fileWithPath );
      size_t fileSize = IO::GetFileSize( fileWithPath );

      if( getAsFile )
      {
        IO::ResourceManager::onFileAsyncThreadCall( fileWithPath.c_str(), userData );
        IO::ResourceManager::onProgressAsyncThreadCall( extension.c_str(), fileSize, fileSize, userData );
      }
      else
      {
        std::ifstream file( fileWithPath.c_str(), std::ios::in | std::ios::binary );
        if( !file.is_open() )
          IO::ResourceManager::onFailureAsyncThreadCall( "Unable to open file", userData );

        file.exceptions( std::ifstream::badbit );
        try
        {
          size_t fileSize = IO::GetFileSize( fileWithPath.c_str() );
          static const size_t maxReadSize = 1<<16;//64K buffers
          char data[maxReadSize];
          size_t offset = 0;
          while( offset != fileSize )
          {
            size_t nbRead = std::min( maxReadSize, fileSize-offset );
            file.read( data, nbRead );
            IO::ResourceManager::onDataAsyncThreadCall( offset, nbRead, data, userData );
            offset += nbRead;
            IO::ResourceManager::onProgressAsyncThreadCall( "text/plain", offset, fileSize, userData );
          }
        }
        catch(...)
        {
          IO::ResourceManager::onFailureAsyncThreadCall( "Error while reading file", userData );
        }
      }
    }

    TestSynchronousFileResourceProvider::TestSynchronousFileResourceProvider()
    {
    }

    RC::Handle<IOManager> IOManager::Create(
        IO::ScheduleAsyncCallbackFunc scheduleFunc,
        void *scheduleFuncUserData
        )
    {
      return new IOManager( scheduleFunc, scheduleFuncUserData );
    }

    std::string IOManager::queryUserFilePath(
      bool existingFile,
      std::string const &title,
      std::string const &defaultFilename,
      std::string const &extension
      ) const
    {
      //For unit test purposes only
      if ( !IO::DirExists( "TMP" ) )
	        IO::CreateDir( "TMP" );
      if(existingFile)
        return IO::JoinPath("TMP", "default.txt");
      else
        return IO::JoinPath("TMP", defaultFilename);
    }
  
    IOManager::IOManager(
        IO::ScheduleAsyncCallbackFunc scheduleFunc,
        void *scheduleFuncUserData
        )
        : IO::Manager( IO::SimpleFileHandleManager::Create(), scheduleFunc, scheduleFuncUserData )
    {
      getResourceManager()->registerProvider( RC::Handle<IO::ResourceProvider>::StaticCast( IO::FileResourceProvider::Create( true ) ), true );
      getResourceManager()->registerProvider( RC::Handle<IO::ResourceProvider>::StaticCast( TestSynchronousFileResourceProvider::Create() ), false );
    }
  };
};
