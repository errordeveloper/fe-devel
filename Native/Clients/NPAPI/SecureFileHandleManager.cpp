/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "SecureFileHandleManager.h"
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/Util/Base64.h>
#include <Fabric/Core/Util/Random.h>
#include <string.h>

namespace Fabric
{
  namespace NPAPI
  {
    char const* secureFileHandleScheme = "fabricio";
    char const* secureFileHandleSchemeAndSeparator = "fabricio://";

    RC::Handle<SecureFileHandleManager> SecureFileHandleManager::Create()
    {
      return new SecureFileHandleManager();
    }

    SecureFileHandleManager::SecureFileHandleManager()
      : m_encodedHandleLength(0)
    {
    }

    char const *SecureFileHandleManager::getUrlScheme() const
    {
      return secureFileHandleScheme;
    }

    std::string SecureFileHandleManager::createHandle( std::string const &path, bool folder, bool readOnly )
    {
      Data data;
      data.m_path = path;
      data.m_isFolder = folder;
      data.m_readOnly = readOnly;

      IO::validateAbsolutePath( data.m_path );//Ensure no '..'

      std::pair< PathToHandleMap::const_iterator, PathToHandleMap::const_iterator > existingRange = m_pathToHandle.equal_range( data.m_path );
      while( existingRange.first != existingRange.second )
      {
        const Data& existingData = existingRange.first->second->second;
        if( existingData.m_isFolder == data.m_isFolder && existingData.m_readOnly == data.m_readOnly )
          return existingRange.first->second->first;
        ++existingRange.first;
      }

      std::string handle;
      while( true )
      {
        static const size_t folderIDByteCount = 24;
        uint8_t folderIDBytes[folderIDByteCount];
        Util::generateSecureRandomBytes( folderIDByteCount, folderIDBytes );
        handle = std::string( secureFileHandleSchemeAndSeparator ) + Util::encodeBase64( folderIDBytes, folderIDByteCount );

        if( m_encodedHandleLength == 0 )
          m_encodedHandleLength = handle.size();
        else
        {
          FABRIC_ASSERT( m_encodedHandleLength == handle.size() );
        }

        if( m_handleToData.find( handle ) == m_handleToData.end() )
          break;

        //If we are here, either we are really not lucky, or the random algo is really bad!
      }

      std::pair<HandleToDataMap::iterator, bool> result = m_handleToData.insert( std::make_pair( handle, data ) );
      m_pathToHandle.insert( std::make_pair( data.m_path, result.first ) );
      return handle;
    }

    std::string SecureFileHandleManager::createRelativeHandle( std::string const &handlePlusRelativePath, bool folder )
    {
      std::string relativePathPostfix;
      Data const &data = validateHandleAndGetData( handlePlusRelativePath, relativePathPostfix );
      if( relativePathPostfix.empty() )
      {
        if( folder != data.m_isFolder )
          throw Exception( "Invalid: trying to change a Folder handle to a File handle or vice-versa" );
        return handlePlusRelativePath;
      }
      FABRIC_ASSERT( data.m_isFolder );//checked by validateHandleAndGetData
      return createHandle( data.m_path + relativePathPostfix, folder, data.m_readOnly );
    }

    bool SecureFileHandleManager::isValid( std::string const &handle ) const
    {
      if( handle.length() < m_encodedHandleLength )
        return false;

      HandleToDataMap::const_iterator iter = m_handleToData.find( std::string( handle, 0, m_encodedHandleLength ) );
      if( iter == m_handleToData.end() )
        return false;

      return handle.length() == m_encodedHandleLength || ( iter->second.m_isFolder && handle[m_encodedHandleLength] == '/' );
    }

    bool SecureFileHandleManager::hasRelativePath( std::string const &handle ) const
    {
      return handle.length() > m_encodedHandleLength;
    }

    SecureFileHandleManager::Data const &SecureFileHandleManager::validateHandleAndGetData( std::string const &handle, std::string& relativePathPostfix ) const
    {
      if( handle.length() < m_encodedHandleLength )
        throw Exception( "Invalid FileHandle" );

      HandleToDataMap::const_iterator iter = m_handleToData.find( handle.substr( 0, m_encodedHandleLength ) );
      if( iter == m_handleToData.end() )
        throw Exception( "Invalid FileHandle" );

      if( handle.length() > m_encodedHandleLength )
      {
        if( !iter->second.m_isFolder )
          throw Exception( "File handles cannot have relative paths" );
        relativePathPostfix.assign( handle.data() + m_encodedHandleLength, handle.length() - m_encodedHandleLength );

        if( relativePathPostfix[0] != '/' )
          throw Exception( "FileHandle relative paths must start with '/'" );

        relativePathPostfix = IO::ChangeSeparatorsURLToFile( relativePathPostfix );
        IO::validateAbsolutePath( relativePathPostfix );//ensure no '..'
      }
      return iter->second;
    }

    std::string SecureFileHandleManager::getPath( std::string const &handle ) const
    {
      std::string relativePathPostfix;
      Data const &data = validateHandleAndGetData( handle, relativePathPostfix );
      return relativePathPostfix.empty() ? data.m_path : data.m_path + relativePathPostfix;
    }

    bool SecureFileHandleManager::isReadOnly( std::string const &handle ) const
    {
      std::string relativePathPostfix;
      Data const &data = validateHandleAndGetData( handle, relativePathPostfix );
      return data.m_readOnly;
    }

    bool SecureFileHandleManager::isFolder( std::string const &handle ) const
    {
      std::string relativePathPostfix;
      Data const &data = validateHandleAndGetData( handle, relativePathPostfix );
      if( relativePathPostfix.empty() )
        return data.m_isFolder;
      FABRIC_ASSERT( data.m_isFolder );//checked by validateHandleAndGetData
      std::string fullPath = data.m_path + relativePathPostfix;
      return IO::DirExists( fullPath );
    }
  };
};
