/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "SimpleFileHandleManager.h"
#include <Fabric/Base/Exception.h>
#include <Fabric/Core/IO/Helpers.h>
#include <string.h>

namespace Fabric
{
  namespace IO
  {
    std::string fileHandleScheme( "file" );
    std::string fileHandleSchemeAndSeparator( "file://" );

    RC::Handle<SimpleFileHandleManager> SimpleFileHandleManager::Create()
    {
      return new SimpleFileHandleManager();
    }

    SimpleFileHandleManager::SimpleFileHandleManager()
    {
    }

    char const *SimpleFileHandleManager::getUrlScheme() const
    {
      return fileHandleScheme.c_str();
    }

    std::string SimpleFileHandleManager::makeHandle( std::string const& path ) const
    {
      if( path.find( fileHandleSchemeAndSeparator ) == 0 )
        return path;
      return fileHandleSchemeAndSeparator + ChangeSeparatorsFileToURL( path );
    }

    std::string SimpleFileHandleManager::createHandle( std::string const &path, bool folder, bool readOnly )
    {
      Data data;
      data.m_isFolder = folder;
      data.m_readOnly = readOnly;
      std::string handle = makeHandle( path );

      std::pair<HandleToDataMap::iterator, bool> result = m_handleToData.insert( std::make_pair( handle, data ) );
      if( result.first != m_handleToData.end() )
      {
        Data& existingData = result.first->second;
        if( existingData.m_isFolder != data.m_isFolder )
            throw Exception( "Invalid: trying to change a Folder handle to a File handle or vice-versa" );
        if( existingData.m_readOnly != data.m_readOnly )
          existingData.m_readOnly = false;//[JeromeCG 20120227] The read only concept is really for secure file handles (to revisit...)
      }
      return handle;
    }

    std::string SimpleFileHandleManager::createRelativeHandle( std::string const &handlePlusRelativePath, bool folder )
    {
      //[JeromeCG 20120227] To refactor; this feature should be secure file handler specific
      return createHandle( handlePlusRelativePath, folder, false );
    }

    bool SimpleFileHandleManager::isValid( std::string const &handle ) const
    {
      //This function is more relevant for secure file handles.
      if( handle.find( fileHandleSchemeAndSeparator ) == 0 )
        return true;
      else
        return handle.find( "//:" ) == std::string::npos;
    }

    bool SimpleFileHandleManager::hasRelativePath( std::string const &handle ) const
    {
      //[JeromeCG 20120227] For consistency, just check if we have data related to that handle (this function is more relevant for secure file handles).
      return m_handleToData.find( makeHandle( handle ) ) == m_handleToData.end();
    }

    SimpleFileHandleManager::Data const *SimpleFileHandleManager::getHandleData( std::string const &handle ) const
    {
      HandleToDataMap::const_iterator it = m_handleToData.find( makeHandle( handle ) );
      return it != m_handleToData.end() ? &(it->second) : NULL;
    }

    std::string SimpleFileHandleManager::getPath( std::string const &handle ) const
    {
      if( handle.find( fileHandleSchemeAndSeparator ) == 0 )
        return ChangeSeparatorsURLToFile( handle.substr( fileHandleSchemeAndSeparator.length() ) );
      else
        return ChangeSeparatorsURLToFile( handle );
    }

    bool SimpleFileHandleManager::isReadOnly( std::string const &handle ) const
    {
      SimpleFileHandleManager::Data const *data = getHandleData( handle );
      return data ? data->m_readOnly : false;
    }

    bool SimpleFileHandleManager::isFolder( std::string const &handle ) const
    {
      SimpleFileHandleManager::Data const *data = getHandleData( handle );
      return data ? data->m_isFolder : DirExists( getPath( handle ) );
    }

  };
};
