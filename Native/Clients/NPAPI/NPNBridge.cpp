/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <assert.h>
#include <npapi/npapi.h>
#include <npapi/npfunctions.h>
#include <string.h>

static NPNetscapeFuncs kBrowserFuncs = { 0 };

extern "C" void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions)
{
  memcpy( &kBrowserFuncs, browser_functions, browser_functions->size );
}

bool NPN_Construct( NPP npp, NPObject* obj, NPVariant const *args, uint32_t argCount, NPVariant *result )
{
  return kBrowserFuncs.construct( npp, obj, args, argCount, result );
}

void NPN_ForceRedraw( NPP instance )
{
  kBrowserFuncs.forceredraw( instance );
}

void NPN_InvalidateRect( NPP instance, NPRect *rect )
{
  kBrowserFuncs.invalidaterect( instance, rect );
}

void NPN_SetException( NPObject *obj, const NPUTF8 *message )
{
  kBrowserFuncs.setexception( obj, message );
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  return kBrowserFuncs.getvalue(instance, variable, value);
}

NPError NPN_SetValue( NPP instance, NPPVariable variable, void *value )
{
  return kBrowserFuncs.setvalue( instance, variable, value );
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8* name)
{
  return kBrowserFuncs.getstringidentifier(name);
}

void* NPN_MemAlloc( uint32_t size )
{
  return kBrowserFuncs.memalloc(size);
}

void NPN_MemFree( void* mem )
{
  kBrowserFuncs.memfree(mem);
}

NPObject* NPN_CreateObject( NPP npp, NPClass* np_class )
{
  return kBrowserFuncs.createobject(npp, np_class);
}

NPObject* NPN_RetainObject( NPObject* obj )
{
  return kBrowserFuncs.retainobject(obj);
}

void NPN_ReleaseObject( NPObject* obj )
{
  kBrowserFuncs.releaseobject(obj);
}

void NPN_ReleaseVariantValue( NPVariant *npVariant )
{
  kBrowserFuncs.releasevariantvalue( npVariant );
}

NPUTF8 *NPN_UTF8FromIdentifier( NPIdentifier identifier )
{
  return kBrowserFuncs.utf8fromidentifier( identifier );
}

bool NPN_Enumerate( NPP npp, NPObject *npobj, NPIdentifier **identifier, uint32_t *count)
{
  return kBrowserFuncs.enumerate(npp, npobj, identifier, count);
}

bool NPN_Invoke( NPP npp, NPObject *npobj, NPIdentifier method, NPVariant const *args, uint32_t argCount, NPVariant *result )
{
  return kBrowserFuncs.invoke(npp, npobj, method, args, argCount, result);
}

bool NPN_InvokeDefault( NPP npp, NPObject *npobj, NPVariant const *args, uint32_t argCount, NPVariant *result )
{
  return kBrowserFuncs.invokeDefault(npp, npobj, args, argCount, result);
}

bool NPN_GetProperty( NPP npp, NPObject *npobj, NPIdentifier property_name, NPVariant *result )
{
  return kBrowserFuncs.getproperty(npp, npobj, property_name, result);
}

bool NPN_RemoveProperty( NPP npp, NPObject *npobj, NPIdentifier property_name )
{
  return kBrowserFuncs.removeproperty(npp, npobj, property_name);
}

bool NPN_SetProperty(NPP npp, NPObject *npobj, NPIdentifier property_name,
                     const NPVariant *value) {
  return kBrowserFuncs.setproperty(npp, npobj, property_name, value);
}

bool NPN_HasProperty( NPP npp, NPObject *npObject, NPIdentifier property_name )
{
  return kBrowserFuncs.hasproperty(npp, npObject, property_name);
}

bool NPN_HasMethod( NPP npp, NPObject *npObject, NPIdentifier methodId )
{
  return kBrowserFuncs.hasmethod( npp, npObject, methodId );
}

bool NPN_Evaluate( NPP npp, NPObject *npobj, NPString *script, NPVariant *result )
{
  return kBrowserFuncs.evaluate(npp, npobj, script, result);
}

NPIdentifier NPN_GetIntIdentifier( int32_t intid )
{
  return kBrowserFuncs.getintidentifier(intid);
}

bool NPN_IdentifierIsString( NPIdentifier id )
{
  return kBrowserFuncs.identifierisstring(id);
}

void NPN_PluginThreadAsyncCall( NPP npp, void (*func)(void *), void *user_data )
{
  kBrowserFuncs.pluginthreadasynccall(npp, func, user_data);
}

uint32_t NPN_ScheduleTimer( NPP npp, uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID) )
{
  if ( kBrowserFuncs.scheduletimer )
    return kBrowserFuncs.scheduletimer( npp, interval, repeat, timerFunc );
  else return (uint32_t)-1;
}

void NPN_UnscheduleTimer( NPP npp, uint32_t timerID )
{
  if ( kBrowserFuncs.unscheduletimer && timerID != (uint32_t)-1 )
    kBrowserFuncs.unscheduletimer( npp, timerID );
}

NPError NPN_GetURL(NPP npp, const char *url, const char *target)
{
  return kBrowserFuncs.geturl( npp, url, target );
}

NPError NPN_GetURLNotify(NPP npp, const char *url, const char *target, void *notifyData)
{
  return kBrowserFuncs.geturlnotify( npp, url, target, notifyData );
}

NPError NPN_PopUpContextMenu( NPP npp, NPMenu *npMenu )
{
  return kBrowserFuncs.popupcontextmenu( npp, npMenu );
}
