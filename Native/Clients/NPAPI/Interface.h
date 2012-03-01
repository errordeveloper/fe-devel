/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_INTERFACE_H
#define _FABRIC_NPAPI_INTERFACE_H

#include <Fabric/Core/DG/Client.h>
#include <Fabric/Clients/NPAPI/ViewPort.h>

namespace Fabric
{
  namespace JSON
  {
    class Value;
    class Object;
    class Array;
  };
  
  namespace NPAPI
  {
    class Context;
    
    class Interface : public DG::Client
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::Handle<Interface> Create( NPP npp, RC::Handle<Context> const &context );
      
      virtual void notify( Util::SimpleString const &jsonEncodedNotifications ) const;
      
      void setViewPort( RC::Handle<ViewPort> const &viewPort );
      
      NPP getNPP() const
      {
        return m_npp;
      }
      RC::Handle<Context> getContext() const;
      
      NPError nppGetValue( NPP npp, NPPVariable variable, void *value );
      NPError nppSetWindow( NPP npp, NPWindow *window );
      int16_t nppHandleEvent( NPP npp, void* event );
      NPError nppDestroy( NPP npp, NPSavedData** save );

      NPError nppNewStream( NPP npp, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype );
      int32_t nppWriteReady( NPP npp, NPStream* stream );
      int32_t nppWrite( NPP npp, NPStream* stream, int32_t offset, int32_t len, void* buffer );
      NPError nppDestroyStream( NPP npp, NPStream *stream, NPReason reason );
      NPError nppStreamAsFile( NPP npp, NPStream *stream, const char* fname );
      
    protected:
    
      Interface( NPP npp, RC::Handle<Context> const &context );
      ~Interface();
          
      void jsonExec( char const *jsonEncodedCommandsData, size_t jsonEncodedCommandsLength,
        JSON::Encoder &resultEncoder
        );
      void setJSONNotifyCallback( NPObject *npObject );
      
    private:
    
      class NPInterfaceObject;
            
      NPP m_npp;
      
      RC::Handle<Context> m_context;
      RC::Handle<ViewPort> m_viewPort;
      
      NPObject *m_callbackNPObject;
    };
  };
};

#endif //_FABRIC_NPAPI_INTERFACE_H
