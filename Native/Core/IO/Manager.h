/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_MANAGER_H
#define _FABRIC_IO_MANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <map>
#include <vector>

namespace Fabric
{
  namespace Util
  {
    class JSONGenerator;
    class JSONArrayGenerator;
  };
  
  namespace JSON
  {
    class ArrayEncoder;
    class Encoder;
  };

  namespace IO
  {
    class ResourceManager;
    class FileHandleManager;
    typedef void (*ScheduleAsyncCallbackFunc)( void* scheduleUserData, void (*callbackFunc)(void *), void *callbackFuncUserData );

    class Manager : public RC::Object
    {

    public:
      REPORT_RC_LEAKS
    
      RC::Handle<ResourceManager> getResourceManager() const;
      RC::Handle<FileHandleManager> getFileHandleManager() const;

      virtual void jsonRoute( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );

    protected:
      Manager( RC::Handle<FileHandleManager> fileHandleManager, ScheduleAsyncCallbackFunc scheduleFunc, void *scheduleFuncUserData );

      virtual std::string queryUserFilePath(
        bool existingFile,
        std::string const &title,
        std::string const &defaultFilename,
        std::string const &extension
        ) const = 0;

    private:
      void jsonQueryUserFile( JSON::Entity const &arg, bool *existingFile, const char *defaultExtension, std::string& fullPath, bool& writeAccess, bool queryFolder ) const;

      void jsonExecGetFileInfo( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecQueryUserFileAndFolder( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecQueryUserFile( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecCreateFileHandleFromRelativePath( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecCreateFolderHandleFromRelativePath( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecPutTextFileContent( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecGetTextFileContent( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;

      RC::Handle<ResourceManager> m_resourceManager;
      RC::Handle<FileHandleManager> m_fileHandleManager;
    };
  };
};

#endif //_FABRIC_IO_MANAGER_H
