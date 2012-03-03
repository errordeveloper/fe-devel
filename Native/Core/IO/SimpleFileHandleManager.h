/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_SIMPLEFILEHANDLEMANAGER_H
#define _FABRIC_IO_SIMPLEFILEHANDLEMANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <map>
#include <string>

namespace Fabric
{
  namespace IO
  {
    class SimpleFileHandleManager : public FileHandleManager
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<SimpleFileHandleManager> Create();
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
        bool m_isFolder;
        bool m_readOnly;
      };

      typedef std::map< std::string, Data > HandleToDataMap;

      SimpleFileHandleManager();

      std::string makeHandle( std::string const& path ) const;
      Data const *getHandleData( std::string const &handle ) const;

      HandleToDataMap m_handleToData;
    };
  };
};

#endif //_FABRIC_IO_SIMPLEFILEHANDLEMANAGER_H
