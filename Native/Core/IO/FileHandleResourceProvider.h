/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_FILEHANDLERESOURCEPROVIDER_H
#define _FABRIC_IO_FILEHANDLERESOURCEPROVIDER_H

#include <Fabric/Core/IO/ResourceManager.h>

namespace Fabric
{
  namespace IO
  {
    class FileHandleManager;
    class FileResourceProvider;

    class FileHandleResourceProvider : public IO::ResourceProvider
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<FileHandleResourceProvider> Create( RC::Handle<FileHandleManager> fileHandleManager );

      virtual char const * getUrlScheme() const;
      virtual void get( char const *url, bool getAsFile, void* userData );

    private:
      FileHandleResourceProvider( RC::Handle<FileHandleManager> fileHandleManager );
      RC::Handle<FileHandleManager> m_fileHandleManager;
      RC::Handle<FileResourceProvider> m_fileResourceProvider;
    };
  };
};

#endif //_FABRIC_IO_FILEHANDLERESOURCEPROVIDER_H
