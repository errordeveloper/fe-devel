/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_FILEHANDLEMANAGER_H
#define _FABRIC_IO_FILEHANDLEMANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <map>
#include <string>

namespace Fabric
{
  namespace IO
  {
    class FileHandleManager : public RC::Object
    {
    public:

      virtual char const * getUrlScheme() const = 0;

      //Note: createHandle does no check if the file/folder exists
      virtual std::string createHandle( std::string const &path, bool folder, bool readOnly ) = 0;
      virtual std::string createRelativeHandle( std::string const &handlePlusRelativePath, bool folder ) = 0;

      //Note: the following methods supports handle with relative path postfix when applicable
      virtual bool isValid( std::string const &handle ) const = 0;
      virtual bool hasRelativePath( std::string const &handle ) const = 0;
      virtual std::string getPath( std::string const &handle ) const = 0;
      virtual bool isReadOnly( std::string const &handle ) const = 0;
      virtual bool isFolder( std::string const &handle ) const = 0;

      bool targetExists( std::string const &handle ) const;
      void ensureTargetExists( std::string const &handle ) const;

      void putFile( std::string const &handle, size_t size, const void* data, bool append ) const;
      void copyFile( std::string const &source, std::string const &target ) const;

    protected:
      FileHandleManager();

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

      Data const &validateHandleAndGetData( std::string const &handle, std::string& relativePathPostfix ) const;
      std::string getPathInternal( Data const &data, std::string const &relativePathPostfix ) const;
      bool isReadOnlyInternal( Data const &data, std::string const &relativePathPostfix ) const;
      bool isFolderInternal( Data const &data, std::string const &relativePathPostfix ) const;
      bool targetExistsInternal( Data const &data, std::string const &relativePathPostfix ) const;
      void ensureTargetExistsInternal( Data const &data, std::string const &relativePathPostfix ) const;

      size_t m_encodedHandleLength;
      HandleToDataMap m_handleToData;
      PathToHandleMap m_pathToHandle;
    };
  };
};

#endif //_FABRIC_IO_FILEHANDLEMANAGER_H
