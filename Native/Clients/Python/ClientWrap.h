/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PYTHON_CLIENT_WRAP_H
#define _FABRIC_PYTHON_CLIENT_WRAP_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Core/Util/Cond.h>
#include <Fabric/Core/Util/TLS.h>

#include <map>

namespace Fabric
{
  namespace Python
  {
    class Client;

    class ClientWrap : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      ClientWrap();
      virtual ~ClientWrap();

      virtual void notify( Util::SimpleString const &jsonEncodedNotifications ) const;
      void setJSONNotifyCallback( void (*callback)(const char *) );
      void jsonExecAndAllocCStr( char const *data, const size_t length, const char **str );
      void freeJsonCStr( const char *str );
      void runScheduledCallbacks();

    protected:
   
      static void ScheduleAsyncUserCallback(
          void *scheduleUserData,
          void (*callbackFunc)(void *),
          void *callbackFuncUserData
          );
      void runScheduledCallbacksNotify();

      void (*m_notifyCallback)(const char *);

      // map of const char * pointers that we have passed to Python
      // and their associated SimpleString, we can't delete these
      // until Python says it's finished with them
      typedef std::map<const char *, Util::SimpleString *> PassedStringMap;
      PassedStringMap m_passedStrings;

      RC::Handle<Client> m_client;

      Util::TLSVar<bool> m_mainThreadTLS;
      Util::Mutex m_mutex;
      struct AsyncCallbackData
      {
        void (*m_callbackFunc)(void *);
        void *m_callbackFuncUserData;
      };
      std::vector<AsyncCallbackData> m_bufferedAsyncUserCallbacks;
    };
  };
};

#endif //_FABRIC_PYTHON_CLIENT_WRAP_H
