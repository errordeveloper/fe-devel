/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PYTHON_CLIENT_H
#define _FABRIC_PYTHON_CLIENT_H

#include <Fabric/Core/DG/Client.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Core/Util/Cond.h>
#include <Fabric/Core/Util/TLS.h>

#include <map>

namespace Fabric
{
  namespace Python
  {
    class ClientWrap;

    class Client : public DG::Client
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Client> Create( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap );
      virtual void notify( Util::SimpleString const &jsonEncodedNotifications ) const;

      // only here to change visibility to public
      void notifyInitialState() const
      {
        DG::Client::notifyInitialState();
      }

    protected:
   
      Client( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap );
      virtual ~Client();

      ClientWrap *m_clientWrap;
    };
  };
};

#endif //_FABRIC_PYTHON_CLIENT_H
