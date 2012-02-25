/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_V8_CLIENT_H
#define _FABRIC_V8_CLIENT_H

#include <Fabric/Core/DG/Client.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Core/Util/TLS.h>

#include <v8.h>
#include <uv.h>
#include <handle_wrap.h>
#include <vector>
#include <string>

namespace Fabric
{
  namespace CLI
  {
    class ClientWrap;
    
    class Client : public DG::Client
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Client> Create( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap );
      
      virtual void notify( Util::SimpleString const &jsonEncodedNotifications ) const;
      void notifyInitialState() const
      {
        DG::Client::notifyInitialState();
      }
      
      void invalidate();
      
    protected:
    
      Client( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap );
      ~Client();
      
    private:
    
      ClientWrap *m_clientWrap;
    };
    
    class ClientWrap : public node::HandleWrap
    {
    public:
    
      static void Initialize( v8::Handle<v8::Object> target );
      
      void notify( char const *data, size_t length );
      
    protected:
    
      ClientWrap( v8::Handle<v8::Object> target );
      ~ClientWrap();
      
      // V8 callbacks
      static v8::Handle<v8::Value> New( v8::Arguments const &args );
      static v8::Handle<v8::Value> JSONExec( v8::Arguments const &args );
      static v8::Handle<v8::Value> SetJSONNotifyCallback( v8::Arguments const &args );
      static v8::Handle<v8::Value> Close( v8::Arguments const &args );

      // libuv callbacks
      static void AsyncCallback( uv_async_t *async, int status );
    
      // user callbacks
      static void ScheduleAsyncUserCallback( void* scheduleUserData, void (*callbackFunc)(void *), void *callbackFuncUserData );
            
    private:
    
      RC::Handle<Client> m_client;
      Util::Mutex m_mutex;
      Util::TLSVar<bool> m_mainThreadTLS;
      uv_async_t m_uvAsync;
      std::vector<std::string> m_bufferedNotifications;
      struct AsyncCallbackData
      {
        void (*m_callbackFunc)(void *);
        void *m_callbackFuncUserData;
      };
      std::vector<AsyncCallbackData> m_bufferedAsyncUserCallbacks;
      v8::Persistent<v8::Function> m_notifyCallback;
    };
  };
};

#endif //_FABRIC_V8_CLIENT_H
