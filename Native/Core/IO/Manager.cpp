/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/Exception.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/IO/ResourceManager.h>
#include <Fabric/Core/IO/Helpers.h>

#include <fstream>

namespace Fabric
{
  namespace IO
  {
    Manager::Manager( RC::Handle<FileHandleManager> fileHandleManager, ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData )
      : m_resourceManager( ResourceManager::Create( scheduleFunc, scheduleFuncUserData ) )
      , m_fileHandleManager( fileHandleManager )
    {
    }

    RC::Handle<ResourceManager> Manager::getResourceManager() const
    {
      return m_resourceManager;
    }

    RC::Handle<FileHandleManager> Manager::getFileHandleManager() const
    {
      return m_fileHandleManager;
    }

    void Manager::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 0 )
        jsonExec( cmd, arg, resultArrayEncoder );
      else throw Exception( "unroutable" );
    }

    void Manager::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "getFileInfo", 11 ) )
        jsonExecGetFileInfo( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "queryUserFileAndFolder", 22 ) )
        jsonExecQueryUserFileAndFolder( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "queryUserFile", 13 ) )
        jsonExecQueryUserFile( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createFileHandleFromRelativePath", 32 ) )
        jsonExecCreateFileHandleFromRelativePath( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createFolderHandleFromRelativePath", 34 ) )
        jsonExecCreateFolderHandleFromRelativePath( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getTextFileContent", 18 ) )
        jsonExecGetTextFileContent( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "putTextFileContent", 18 ) )
        jsonExecPutTextFileContent( arg, resultArrayEncoder );
      else
        throw Exception( "unknown command" );
    }

    void Manager::jsonQueryUserFile(
      JSON::Entity const &arg,
      bool *existingFile,
      const char *defaultExtension,
      std::string& fullPath,
      bool& writeAccess,
      bool queryFolder
      ) const
    {
      std::string defaultFilename;
      std::string extension;
      std::string title;
      bool existing = false, foundExisting = false;
      writeAccess = false;

      if ( existingFile )
      {
        existing = *existingFile;
        writeAccess = !existing;
        foundExisting = true;
      }

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( !existingFile && keyString.stringIs( "existingFile", 12 ) )
          {
            valueEntity.requireBoolean();
            existing = valueEntity.booleanValue();//mandatory in this case
            foundExisting = true;
          }
          else if ( !existingFile && keyString.stringIs( "writeAccess", 11 ) )
          {
            valueEntity.requireBoolean();
            writeAccess = valueEntity.booleanValue();
          }
          else if ( keyString.stringIs( "uiOptions", 9 ) )
          {
            valueEntity.requireObject();
            JSON::ObjectDecoder uiOptionsObjectDecoder( valueEntity );
            JSON::Entity uiOptionKeyString, uiOptionValueEntity;
            while ( uiOptionsObjectDecoder.getNext( uiOptionKeyString, uiOptionValueEntity ) )
            {
              try
              {
                if ( uiOptionKeyString.stringIs( "defaultFileName", 15 ) )
                {
                  uiOptionValueEntity.requireString();
                  defaultFilename = uiOptionValueEntity.stringToStdString();
                }
                else if ( uiOptionKeyString.stringIs( "extension", 9 ) )
                {
                  uiOptionValueEntity.requireString();
                  extension = uiOptionValueEntity.stringToStdString();
                }
                else if ( uiOptionKeyString.stringIs( "title", 5 ) )
                {
                  uiOptionValueEntity.requireString();
                  title = uiOptionValueEntity.stringToStdString();
                }
              }
              catch ( Exception e )
              {
                uiOptionsObjectDecoder.rethrow( e );
              }
            }
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      if ( !foundExisting )
        throw Exception( "missing 'existingFile'" );

      if ( !existing && !writeAccess )
        throw Exception("Error: trying to save a file with writeAccess == false");

      if ( existing && defaultExtension == NULL )
        defaultExtension = "*";

      if( extension.empty() && defaultExtension )
        extension = defaultExtension;

      if( title.empty() )
      {
        if( existing )
        {
          if( writeAccess )
            title = "Open File with read & write access to folder...";
          else
            title = "Open File...";
        }
        else
          title = "Save File...";
      }
      else
      {
        //[JCG 20110915] I'm prepending by Open/Save to ensure users can't be mislead, but maybe it's too much control?
        if( existing )
        {
          if( writeAccess )
          {
            if( queryFolder )
              title = "Open File with read & write access to folder: " + title;
            else
              title = "Open File with write access: " + title;
          }
          else
          {
            if( queryFolder )
              title = "Open File with read access to folder: " + title;
            else
              title = "Open File: " + title;
          }
        }
        else
        {
          if( queryFolder )
              title = "Save File with access to folder: " + title;
          else
            title = "Save File: " + title;
        }
      }

      if( !defaultFilename.empty() && !extension.empty() )
      {
        size_t extPos = defaultFilename.rfind( extension );
        if( !(extPos == defaultFilename.size() - extension.size() && extPos > 0 && defaultFilename[ extPos-1 ] == L'.') )
          defaultFilename += '.' + extension;
      }
      fullPath = queryUserFilePath( existing, title, defaultFilename, extension );
    }

    void Manager::jsonExecGetFileInfo( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      arg.requireString();
      std::string handle = arg.stringToStdString();

      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();

      bool isFile = false;
      bool exists = m_fileHandleManager->targetExists( handle );

      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "type", 4 );
        if( m_fileHandleManager->hasRelativePath( handle ) && !exists )
          memberEncoder.makeString( "unknown", 7 );
        else if( m_fileHandleManager->isFolder( handle ) )
          memberEncoder.makeString( "folder", 6 );
        else
        {
          memberEncoder.makeString( "file", 4 );
          isFile = true;
        }
      }

      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "writeAccess", 11 );
        memberEncoder.makeBoolean( !m_fileHandleManager->isReadOnly( handle ) );
      }

      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "exists", 6 );
        memberEncoder.makeBoolean( exists );
      }

      if( isFile )
      {
        std::string fullPath = m_fileHandleManager->getPath( handle );

        std::string dir, filename;
        IO::SplitPath( fullPath, dir, filename );

        {
          JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "fileName", 8 );
          memberEncoder.makeString( filename );
        }

        if( exists )
        {
          JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "fileSize", 8 );
          memberEncoder.makeInteger( (int32_t)GetFileSize( fullPath ) );
        }
      }
    }

    void Manager::jsonExecQueryUserFileAndFolder( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      std::string fullPath;
      bool writeAccess;
      jsonQueryUserFile( arg, NULL, NULL, fullPath, writeAccess, true );

      std::string dir, filename;
      IO::SplitPath( fullPath, dir, filename );

      std::string dirHandle = m_fileHandleManager->createHandle( dir, true, !writeAccess );
      std::string fileHandle = m_fileHandleManager->createHandle( fullPath, false, !writeAccess );

      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();

      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "file", 4 );
        memberEncoder.makeString( fileHandle );
      }
        
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "folder", 6 );
        memberEncoder.makeString( dirHandle );
      }
    }

    void Manager::jsonExecQueryUserFile( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      std::string fullPath;
      bool writeAccess;

      jsonQueryUserFile( arg, NULL, NULL, fullPath, writeAccess, false );

      std::string handle = m_fileHandleManager->createHandle( fullPath, false, !writeAccess );

      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeString( handle );
    }

    void Manager::jsonExecCreateFileHandleFromRelativePath( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      arg.requireString();
      std::string handle = arg.stringToStdString();
      std::string newHandle = m_fileHandleManager->createRelativeHandle( handle, false );

      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeString( newHandle );
    }

    void Manager::jsonExecCreateFolderHandleFromRelativePath( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      arg.requireString();
      std::string handle = arg.stringToStdString();
      std::string newHandle = m_fileHandleManager->createRelativeHandle( handle, true );

      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeString( newHandle );
    }

    void Manager::jsonExecPutTextFileContent( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      std::string content, handle;
      bool append = false;
      bool foundContent = false;

      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity argKeyString, argValueEntity;
      while ( argObjectDecoder.getNext( argKeyString, argValueEntity ) )
      {
        try
        {
          if ( argKeyString.stringIs( "content", 7 ) )
          {
            argValueEntity.requireString();
            content = argValueEntity.stringToStdString();
            foundContent = true;
          }
          else if ( argKeyString.stringIs( "file", 4 ) )
          {
            argValueEntity.requireString();
            handle = argValueEntity.stringToStdString();
          }
          else if ( argKeyString.stringIs( "append", 6 ) )
          {
            argValueEntity.requireBoolean();
            append = argValueEntity.booleanValue();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }

      if ( !foundContent )
        throw Exception( "missing 'content'" );
      if ( handle.empty() )
        throw Exception( "missing 'file'" );

      m_fileHandleManager->putFile( handle, content.length(), content.data(), append );
    }

    void Manager::jsonExecGetTextFileContent( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      arg.requireString();
      std::string handle = arg.stringToStdString();

      std::string fullPath = m_fileHandleManager->getPath( handle );

      if( !FileExists( fullPath ) )
        throw Exception( "Error: file not found" );

      //Note: we don't request file data from ResourceManager beause we need a synchronous response
      size_t fileSize = GetFileSize( fullPath );
      std::string content;
      content.resize( fileSize );

      std::ifstream file( fullPath.c_str(), std::ios::in );
      if( !file.is_open() )
        throw Exception( "Unable to open file" );

      file.read( (char*)content.data(), fileSize );
      if( file.bad() )
        throw Exception( "Error while reading text file" );

      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeString( content );
    }
  };
};
