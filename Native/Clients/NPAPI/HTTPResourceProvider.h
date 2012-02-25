/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_HTTP_RESOURCE_PROVIDER_H
#define _FABRIC_NPAPI_HTTP_RESOURCE_PROVIDER_H

#include <Fabric/Core/IO/ResourceManager.h>
#include <npapi/npapi.h>

namespace Fabric
{
  namespace NPAPI
  {
    class HTTPResourceProvider : public IO::ResourceProvider
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<HTTPResourceProvider> Create( NPP npp );

      virtual char const * getUrlScheme() const;
      virtual void get( char const *url, bool getAsFile, void* userData );

      NPError nppNewStream( NPP npp, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype );
      int32_t nppWriteReady( NPP npp, NPStream* stream );
      int32_t nppWrite( NPP npp, NPStream* stream, int32_t offset, int32_t len, void* buffer );
      NPError nppDestroyStream( NPP npp, NPStream *stream, NPReason reason );
      NPError nppStreamAsFile( NPP npp, NPStream *stream, const char* fname );

    private:
      HTTPResourceProvider( NPP npp );
      NPP m_npp;
    };
  };
};

#endif //_FABRIC_NPAPI_HTTP_RESOURCE_PROVIDER_H
