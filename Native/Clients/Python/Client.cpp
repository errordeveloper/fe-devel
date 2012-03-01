/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>
#include <Fabric/Clients/Python/Client.h>
#include <Fabric/Clients/Python/ClientWrap.h>
#include <Fabric/Clients/Python/IOManager.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/ResourceManager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/IO/FileHandleResourceProvider.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/MT/Util.h>

#include <vector>
#include <string>

namespace Fabric
{
  namespace Python
  {
    RC::Handle<Client> Client::Create( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap )
    {
      return new Client( context, clientWrap );
    }
    
    Client::Client( RC::Handle<DG::Context> const &context, ClientWrap *clientWrap )
      : DG::Client( context )
      , m_clientWrap( clientWrap )
    {
    }

    Client::~Client()
    {
    }

    void Client::notify( Util::SimpleString const &jsonEncodedNotifications ) const
    {
      FABRIC_ASSERT( m_clientWrap );
      m_clientWrap->notify( jsonEncodedNotifications );
    }
  }
};
