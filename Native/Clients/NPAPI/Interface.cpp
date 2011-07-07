/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Clients/NPAPI/Interface.h>
#include <Fabric/Core/Build.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Clients/NPAPI/IOManager.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/JSON/Array.h>
#include <Fabric/Base/JSON/Decode.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Core/Util/Assert.h>

namespace Fabric
{
  namespace NPAPI
  {
    class Interface::NPInterfaceObject : public NPObject
    {
    public:
    
      NPInterfaceObject()
        : m_jsonExecNPIdentifier( NPN_GetStringIdentifier( "jsonExec" ) )
        , m_setJSONNotifyCallbackNPIdentifier( NPN_GetStringIdentifier( "setJSONNotifyCallback" ) )
      {
      }
      
      RC::Handle<Interface> getInterface() const
      {
        return m_interface;
      }
      
      void setInterface( RC::Handle<Interface> const &interface )
      {
        m_interface = interface;
      }
      
      static NPClass *getNPClass()
      {
        return &s_npClass;
      }
    
    protected:

      static NPObject *NPAllocate( NPP npp, NPClass *npClass )
      {
        return new NPInterfaceObject;
      }
    
      static void NPDeallocate( NPObject *npObject )
      {
        delete static_cast<NPInterfaceObject *>( npObject );
      }
      
      static void NPInvalidate( NPObject *npobj )
      {
      }
      
      bool npHasMethod( NPIdentifier identifier ) const
      {
        return identifier == m_jsonExecNPIdentifier
          || identifier == m_setJSONNotifyCallbackNPIdentifier;
      }
      
      static bool NPHasMethod( NPObject *npObject, NPIdentifier identifier )
      {
        return static_cast<NPInterfaceObject const *>( npObject )->npHasMethod( identifier );
      }
      
      bool npInvoke(
        NPIdentifier identifier,
        NPVariant const *args,
        uint32_t argCount,
        NPVariant *result
        )
      {
        if ( identifier == m_jsonExecNPIdentifier )
        {
          try
          {
            if ( argCount != 1 || !NPVARIANT_IS_STRING( args[0] ) )
              throw Exception( "takes 1 string parameter (jsonEncodedCommands)" );
            NPString const &npString = NPVARIANT_TO_STRING( args[0] );
            
            std::string jsonEncodedResults = m_interface->jsonExec( npString.UTF8Characters, npString.UTF8Length );
            
            char *utf8Characters = (char *)NPN_MemAlloc( jsonEncodedResults.length() );
            memcpy( utf8Characters, jsonEncodedResults.data(), jsonEncodedResults.length() );
            STRINGN_TO_NPVARIANT( utf8Characters, jsonEncodedResults.length(), *result );
          }
          catch ( Exception e )
          {
            NPN_SetException( this, e.getDesc().c_str() );
          }
          catch ( ... )
          {
            NPN_SetException( this, "generic exception" );
          }
          return true;
        }
        else if ( identifier == m_setJSONNotifyCallbackNPIdentifier )
        {
          try
          {
            if ( argCount != 1 || !NPVARIANT_IS_OBJECT( args[0] ) )
              throw Exception( "takes 1 string parameter (callback)" );
            NPObject *npObject = NPVARIANT_TO_OBJECT( args[0] );
            
            m_interface->setJSONNotifyCallback( npObject );
            
            VOID_TO_NPVARIANT( *result );
          }
          catch ( Exception e )
          {
            NPN_SetException( this, e.getDesc().c_str() );
          }
          catch ( ... )
          {
            NPN_SetException( this, "generic exception" );
          }
        }
        else return false;
      }
      
      static bool NPInvoke(
        NPObject *npObject,
        NPIdentifier identifier,
        NPVariant const *args,
        uint32_t argCount,
        NPVariant *result
        )
      {
        return static_cast<NPInterfaceObject *>( npObject )->npInvoke( identifier, args, argCount, result );
      }

      static bool NPInvokeDefault(
        NPObject *npObject,
        NPVariant const *args,
        uint32_t argCount,
        NPVariant *result
        )
      {
        return false;
      }
      
      static bool NPHasProperty( NPObject *npObject, NPIdentifier identifier )
      {
        return false;
      }

      static bool NPGetProperty( NPObject *npobj, NPIdentifier name, NPVariant *result )
      {
        return false;
      }
      
      static bool NPSetProperty( NPObject *npobj, NPIdentifier name, const NPVariant *value )
      {
        return false;
      }
      
      static bool NPRemoveProperty( NPObject *npobj, NPIdentifier name )
      {
        return false;
      }
      
      bool npEnumerate( NPIdentifier **_value, uint32_t *_count ) const
      {
        *_count = 2;
        *_value = (NPIdentifier *)NPN_MemAlloc( 2 * sizeof( NPIdentifier ) );
        *_value[0] = m_jsonExecNPIdentifier;
        *_value[1] = m_setJSONNotifyCallbackNPIdentifier;
        return true;
      }
      
      static bool NPEnumerate( NPObject *npObject, NPIdentifier **value, uint32_t *count )
      {
        return static_cast<NPInterfaceObject *>( npObject )->npEnumerate( value, count );
      }

      static bool NPConstruct( NPObject *npobj, const NPVariant *args, uint32_t argCount, NPVariant *result )
      {
        return false;
      }
      
    private:
    
      RC::Handle<Interface> m_interface;
      
      NPIdentifier m_jsonExecNPIdentifier;
      NPIdentifier m_setJSONNotifyCallbackNPIdentifier;
      
      static NPClass s_npClass;
    };
    
    NPClass Interface::NPInterfaceObject::s_npClass =
    {
      NP_CLASS_STRUCT_VERSION,
      &Interface::NPInterfaceObject::NPAllocate,
      &Interface::NPInterfaceObject::NPDeallocate,
      &Interface::NPInterfaceObject::NPInvalidate,
      &Interface::NPInterfaceObject::NPHasMethod,
      &Interface::NPInterfaceObject::NPInvoke,
      &Interface::NPInterfaceObject::NPInvokeDefault,
      &Interface::NPInterfaceObject::NPHasProperty,
      &Interface::NPInterfaceObject::NPGetProperty,
      &Interface::NPInterfaceObject::NPSetProperty,
      &Interface::NPInterfaceObject::NPRemoveProperty,
      &Interface::NPInterfaceObject::NPEnumerate,
      &Interface::NPInterfaceObject::NPConstruct,
    };

    RC::Handle<Interface> Interface::Create( NPP npp, RC::Handle<Context> const &context )
    {
      return new Interface( npp, context );
    }
    
    Interface::Interface( NPP npp, RC::Handle<Context> const &context )
      : DG::Client( context )
      , m_npp( npp )
      , m_context( context )
      , m_callbackNPObject( 0 )
    {
    }
    
    Interface::~Interface()
    {
      if ( m_callbackNPObject )
        NPN_ReleaseObject( m_callbackNPObject );
    }
    
    void Interface::setViewPort( RC::Handle<ViewPort> const &viewPort )
    {
      m_viewPort = viewPort;
    }
      
    NPError Interface::nppGetValue( NPP npp, NPPVariable variable, void *value )
    {
      FABRIC_ASSERT( npp == m_npp );
      if ( variable == NPPVpluginScriptableNPObject )
      {
        NPInterfaceObject *npInterfaceObject = static_cast<NPInterfaceObject *>( NPN_CreateObject( m_npp, NPInterfaceObject::getNPClass() ) );
        npInterfaceObject->setInterface( this );
        *reinterpret_cast<NPObject**>(value) = npInterfaceObject;
        return NPERR_NO_ERROR;
      }
      else if ( m_viewPort )
        return m_viewPort->nppGetValue( variable, value );
      else return NPERR_INVALID_PARAM;
    }
    
    NPError Interface::nppSetWindow( NPP npp, NPWindow *npWindow )
    {
      FABRIC_ASSERT( npp == m_npp );
      if ( m_viewPort )
        return m_viewPort->nppSetWindow( npWindow );
      else return NPERR_NO_ERROR;
    }
    
    int16_t Interface::nppHandleEvent( NPP npp, void *event )
    {
      FABRIC_ASSERT( npp == m_npp );
      if ( m_viewPort )
        return m_viewPort->nppHandleEvent( event );
      else return false;
    }

    NPError Interface::nppDestroy( NPP npp, NPSavedData** save )
    {
      FABRIC_ASSERT( npp == m_npp );
      if ( m_viewPort )
        return m_viewPort->nppDestroy( save );
      else 
        return NPERR_NO_ERROR;
    }
    
    NPError Interface::nppNewStream( NPP npp, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype )
    {
      FABRIC_ASSERT( npp == m_npp );
      return m_context->getIOManager()->nppNewStream( npp, type, stream, seekable, stype );
    }

    void Interface::nppStreamAsFile( NPP npp, NPStream *stream, const char *fname )
    {
      FABRIC_ASSERT( npp == m_npp );
      m_context->getIOManager()->nppStreamAsFile( npp, stream, fname );
    }

    NPError Interface::nppDestroyStream( NPP npp, NPStream *stream, NPReason reason )
    {
      FABRIC_ASSERT( npp == m_npp );
      return m_context->getIOManager()->nppDestroyStream( npp, stream, reason );
    }
      
    std::string Interface::jsonExec( char const *jsonEncodedCommandsData, size_t jsonEncodedCommandsLength )
    {
      if ( m_viewPort )
        m_viewPort->pushOGLContext();
      std::string result = DG::Client::jsonExec( jsonEncodedCommandsData, jsonEncodedCommandsLength );
      if ( m_viewPort )
        m_viewPort->popOGLContext();
      return result;
    }

    void Interface::setJSONNotifyCallback( NPObject *npObject )
    {
      if ( m_callbackNPObject )
        NPN_ReleaseObject( m_callbackNPObject );
        
      m_callbackNPObject = npObject;
      
      if ( m_callbackNPObject )
      {
        NPN_RetainObject( m_callbackNPObject );
        notifyInitialState();
      }
    }

    RC::Handle<Context> Interface::getContext() const
    {
      return m_context;
    }
      
    void Interface::notify( std::string const &jsonEncodedNotifications ) const
    {
      if ( m_callbackNPObject )
      {
        NPVariant arg;
        char *utf8Characters = (char *)NPN_MemAlloc( jsonEncodedNotifications.length() );
        memcpy( utf8Characters, jsonEncodedNotifications.data(), jsonEncodedNotifications.length() );
        STRINGN_TO_NPVARIANT( utf8Characters, jsonEncodedNotifications.length(), arg );
        
        try
        {
          NPVariant result;
          if ( !NPN_InvokeDefault( m_npp, m_callbackNPObject, &arg, 1, &result ) )
            throw Exception( "JSON notify callback failure" );
          NPN_ReleaseVariantValue( &result );
        }
        catch ( Exception e )
        {
          FABRIC_LOG( "Failed to send notification!" );
        }
      }
    }
  };
};
