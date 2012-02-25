/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_WINDOWED_INVALIDATING_CA_VIEW_PORT_H
#define _FABRIC_NPAPI_WINDOWED_INVALIDATING_CA_VIEW_PORT_H

#include <Fabric/Clients/NPAPI/Darwin/WindowedCAViewPort.h>

#include <npapi/npapi.h>

namespace Fabric
{
  namespace NPAPI
  {
    class WindowedInvalidatingCAViewPort : public WindowedCAViewPort
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ViewPort> Create( RC::ConstHandle<Interface> const &interface );

      virtual void redrawFinished();
      
    protected:
    
      WindowedInvalidatingCAViewPort( RC::ConstHandle<Interface> const &interface );
      
      void sendInvalidateMessage();
      virtual void asyncRedrawFinished();
      
    private:

      NPP m_npp;
    };
  };
};

#endif //_FABRIC_NPAPI_WINDOWED_INVALIDATING_CA_VIEW_PORT_H
