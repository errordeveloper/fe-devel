/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_IO_FILERESOURCEPROVIDER_H
#define _FABRIC_IO_FILERESOURCEPROVIDER_H

#include <Fabric/Core/IO/ResourceManager.h>

namespace Fabric
{
  namespace IO
  {
    //Note: FileResourceProvider should not be registered directly as a public resource provider in secure configurations (client)
    class FileResourceProvider : public IO::ResourceProvider
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<FileResourceProvider> Create();

      virtual char const * getUrlScheme() const;
      virtual void get( char const *url, bool getAsFile, void* userData );

    private:
      FileResourceProvider();
    };
  };
};

#endif //_FABRIC_IO_FILERESOURCEPROVIDER_H
