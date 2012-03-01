/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "FileHandleResourceProvider.h"
#include "FileHandleManager.h"
#include "FileResourceProvider.h"
#include "Helpers.h"
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace IO
  {
    RC::Handle<FileHandleResourceProvider> FileHandleResourceProvider::Create( RC::Handle<FileHandleManager> fileHandleManager )
    {
      return new FileHandleResourceProvider( fileHandleManager);
    }

    FileHandleResourceProvider::FileHandleResourceProvider( RC::Handle<FileHandleManager> fileHandleManager )
      : m_fileHandleManager( fileHandleManager )
      , m_fileResourceProvider( FileResourceProvider::Create( false ) )
    {
    }

    char const* FileHandleResourceProvider::getUrlScheme() const
    {
      return m_fileHandleManager->getUrlScheme();
    }

    void FileHandleResourceProvider::get( char const *url, bool getAsFile, void* userData )
    {
      if( m_fileHandleManager->isFolder( url ) )
          throw Exception( "Error: a folder handle was provided instead of a file handle" );
      if( !m_fileHandleManager->targetExists( url ) )
          throw Exception( "Error: file not found" );

      std::string fileWithPath = "file://" + ChangeSeparatorsFileToURL( m_fileHandleManager->getPath( url ) );
      m_fileResourceProvider->get( fileWithPath.c_str(), getAsFile, userData );
    }
  };
};
