/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_SECUREFILEHANDLEMANAGER_H
#define _FABRIC_IO_SECUREFILEHANDLEMANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <map>
#include <string>

namespace Fabric
{
  namespace NPAPI
  {
    class SecureFileHandleManager : public IO::FileHandleManager
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<SecureFileHandleManager> Create();
      char const * getUrlScheme() const;

      //Note: createHandle does no check if the file/folder exists
      virtual std::string createHandle( std::string const &path, bool folder, bool readOnly );
      virtual std::string createRelativeHandle( std::string const &handlePlusRelativePath, bool folder );

      //Note: the following methods supports handle with relative path postfix when applicable
      bool isValid( std::string const &handle ) const;
      bool hasRelativePath( std::string const &handle ) const;
      std::string getPath( std::string const &handle ) const;
      bool isReadOnly( std::string const &handle ) const;
      bool isFolder( std::string const &handle ) const;

    private:
      struct Data
      {
        Data()
          :m_readOnly(false)
        {}
        std::string m_path;
        bool m_isFolder;
        bool m_readOnly;
      };

      typedef std::map< std::string, Data > HandleToDataMap;
      typedef std::multimap< std::string, HandleToDataMap::iterator > PathToHandleMap;

      SecureFileHandleManager();

      Data const &validateHandleAndGetData( std::string const &handle, std::string& relativePathPostfix ) const;

      size_t m_encodedHandleLength;
      HandleToDataMap m_handleToData;
      PathToHandleMap m_pathToHandle;
    };
  };
};

#endif //_FABRIC_IO_SECUREFILEHANDLEMANAGER_H
