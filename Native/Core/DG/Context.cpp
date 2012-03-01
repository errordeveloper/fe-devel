/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/CodeManager.h>
#include <Fabric/Core/DG/Client.h>
#include <Fabric/Core/DG/NamedObject.h>
#include <Fabric/Core/DG/Node.h>
#include <Fabric/Core/DG/ResourceLoadNode.h>
#include <Fabric/Core/DG/Event.h>
#include <Fabric/Core/DG/EventHandler.h>
#include <Fabric/Core/DG/Operator.h>
#include <Fabric/Core/DG/LogCollector.h>
#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/KL/Compiler.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Core/Util/Random.h>
#include <Fabric/Core/Util/Base64.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Core/Build.h>
#include <Fabric/EDK/Common.h>
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <FabricThirdPartyLicenses/llvm/license.h>
#include <FabricThirdPartyLicenses/llvm/autoconf/license.h>
#include <FabricThirdPartyLicenses/llvm/lib/Support/license.h>
#include <FabricThirdPartyLicenses/llvm/projects/sample/autoconf/license.h>
#include <FabricThirdPartyLicenses/md5/license.h>
#include <FabricThirdPartyLicenses/libpng/license.h>
#include <FabricThirdPartyLicenses/liblas/license.h>
#include <FabricThirdPartyLicenses/teem/license.h>

namespace Fabric
{
  namespace DG
  {
    Util::Mutex Context::s_contextMapMutex("Context::s_contextMapMutex");
    Context::ContextMap Context::s_contextMap;

    RC::Handle<Context> Context::Create(
      RC::Handle<IO::Manager> const &ioManager,
      std::vector<std::string> const &pluginDirs,
      CG::CompileOptions const &compileOptions,
      bool optimizeSynchronously
      )
    {
       return new Context( ioManager, pluginDirs, compileOptions, optimizeSynchronously );
    }
    
    RC::Handle<Context> Context::Bind( std::string const &contextID )
    {
      Util::Mutex::Lock contextMapLock( s_contextMapMutex );
      ContextMap::const_iterator it = s_contextMap.find( contextID );
      if ( it == s_contextMap.end() )
        throw Exception( "no context with ID " + _(contextID) );
      return it->second;
    }
    
    Context::Context(
      RC::Handle<IO::Manager> const &ioManager,
      std::vector<std::string> const &pluginDirs,
      CG::CompileOptions const &compileOptions,
      bool optimizeSynchronously
      )
      : m_mutex( "Context::Context" )
      , m_logCollector( LogCollector::Create( this ) )
      , m_rtManager( RT::Manager::Create( KL::Compiler::Create() ) )
      , m_ioManager( ioManager )
      , m_cgManager( CG::Manager::Create( m_rtManager ) )
      , m_compileOptions( compileOptions )
      , m_codeManager( CodeManager::Create( &m_compileOptions, optimizeSynchronously ) )
      , m_clientsMutex( "Context::m_clients" )
      , m_notificationBracketCount( 0 )
      , m_pendingNotificationsMutex( "pending notifications" )
      , m_pendingNotificationsJSON( 0 )
      , m_gcContainer( this )
      , m_mrInterface( &m_gcContainer, m_rtManager )
      , m_klcInterface( &m_gcContainer, m_cgManager, m_compileOptions )
    {
      registerCoreTypes();
      
      m_rtManager->setJSONCommandChannel( this );

      static const size_t contextIDByteCount = 96;
      uint8_t contextIDBytes[contextIDByteCount];
      Util::generateSecureRandomBytes( contextIDByteCount, contextIDBytes );

      m_contextID = Util::encodeBase64( contextIDBytes, contextIDByteCount );
      Util::Mutex::Lock contextMapLock( s_contextMapMutex );
      s_contextMap.insert( ContextMap::value_type( m_contextID, this ) ).first;
    }
    
    Context::~Context()
    {
      FABRIC_ASSERT( !m_pendingNotificationsJSON );
      
      FABRIC_ASSERT( m_clients.empty() );

      {
        Util::Mutex::Lock contextMapLock( s_contextMapMutex );
        
        s_contextMap.erase( s_contextMap.find( m_contextID ) );
      }
      
      m_rtManager->setJSONCommandChannel( 0 );
    }
    
    std::string const &Context::getContextID() const
    {
      return m_contextID;
    }
    
    void Context::registerClient( Client *client )
    {
      Util::Mutex::Lock clientLock( m_clientsMutex );
      FABRIC_VERIFY( m_clients.insert( client ).second );
    }

    void Context::registerCoreTypes()
    {
      RegisterFabricResourceType( m_rtManager );
    }
    
    void Context::jsonNotify(
      std::vector<std::string> const &srcs,
      char const *cmdData,
      size_t cmdLength,
      Util::SimpleString const *argJSON
      )
    {
      NotificationBracket notificationBracket( this );
      
      Util::SimpleString json;
      {
        JSON::Encoder jsonEncoder( &json );
        JSON::ObjectEncoder jsonObjectEncoder = jsonEncoder.makeObject();
        
        {
          JSON::Encoder srcsEncoder = jsonObjectEncoder.makeMember( "src", 3 );
          JSON::ArrayEncoder srcsArrayEncoder = srcsEncoder.makeArray();
          for ( std::vector<std::string>::const_iterator it=srcs.begin(); it!=srcs.end(); ++it )
          {
            JSON::Encoder srcEncoder = srcsArrayEncoder.makeElement();
            srcEncoder.makeString( *it );
          }
        }
        
        {
          JSON::Encoder cmdEncoder = jsonObjectEncoder.makeMember( "cmd", 3 );
          cmdEncoder.makeString( cmdData, cmdLength );
        }
        
        if ( argJSON )
        {
          JSON::Encoder argEncoder = jsonObjectEncoder.makeMember( "arg", 3 );
          argEncoder.appendJSON( *argJSON );
        }
      }
      
      Util::Mutex::Lock pendingNotificationsMutexLock( m_pendingNotificationsMutex );
      if ( !m_pendingNotificationsJSON )
      {
        m_pendingNotificationsJSON = new Util::SimpleString;
        m_pendingNotificationsEncoder = new JSON::Encoder( m_pendingNotificationsJSON );
        m_pendingNotificationsArrayEncoder = m_pendingNotificationsEncoder->newArray();
      }
      m_pendingNotificationsArrayEncoder->makeElement().appendJSON( json );
    }
    
    void Context::unregisterClient( Client *client )
    {
      Util::Mutex::Lock clientLock( m_clientsMutex );
      Clients::iterator it = m_clients.find( client );
      FABRIC_ASSERT( it != m_clients.end() );
      m_clients.erase( it );
    }

    void Context::openNotificationBracket()
    {
      m_notificationBracketCount.increment();
    }
    
    void Context::closeNotificationBracket()
    {
      if ( m_notificationBracketCount.decrementAndGetValue() == 0
        && m_pendingNotificationsJSON )
      {
        Util::SimpleString *pendingNotificationJSON;
        {
          Util::Mutex::Lock pendingNotificationsMutexLock( m_pendingNotificationsMutex );
          pendingNotificationJSON = m_pendingNotificationsJSON;
          delete m_pendingNotificationsArrayEncoder;
          delete m_pendingNotificationsEncoder;
          m_pendingNotificationsJSON = 0;
        }

        {
          Util::Mutex::Lock clientLock( m_clientsMutex );
          // [pzion 20111130] This is a bit insane: in a strange case I haven't
          // tracked down yet, notifying a client can cause it to become
          // unregistered.  We work around this problem by saving the next iterator.
          Clients::const_iterator it = m_clients.begin();
          while( it != m_clients.end() )
          {
            Client *client = *it++;
            client->notify( *pendingNotificationJSON );
          }
        }

        delete pendingNotificationJSON;
      }
    }
    
    RC::Handle<MT::LogCollector> Context::getLogCollector() const
    {
      return m_logCollector;
    }
    
    RC::Handle<RT::Manager> Context::getRTManager() const
    {
      return m_rtManager;
    }
    
    RC::Handle<CG::Manager> Context::getCGManager() const
    {
      return m_cgManager;
    }
    
    RC::Handle<IO::Manager> Context::getIOManager() const
    {
      return m_ioManager;
    }
    
    RC::Handle<CodeManager> Context::getCodeManager() const
    {
      return m_codeManager;
    }
    
    Context::NamedObjectMap &Context::getNamedObjectRegistry() const
    {
      return m_namedObjectRegistry;
    }
              
    RC::Handle<NamedObject> Context::getNamedObject( std::string const &name ) const
    {
      NamedObjectMap::const_iterator it = m_namedObjectRegistry.find( name );
      if ( it == m_namedObjectRegistry.end() )
        throw Exception( "no NamedObject named " + _(name) );
      return it->second;
    }
    
    RC::Handle<Container> Context::getContainer( std::string const &name ) const
    {
      RC::Handle<Container> container;
      try
      {
        RC::Handle<NamedObject> namedObject = getNamedObject( name );
        if ( !namedObject->isContainer() )
          throw Exception("");
        container = RC::Handle<Container>::StaticCast( namedObject );
      }
      catch ( Exception e )
      {
        throw Exception( "no Container named " + _(name) );
      }
      return container;
    }
    
    RC::Handle<Operator> Context::getOperator( std::string const &name ) const
    {
      RC::Handle<Operator> operator_;
      try
      {
        RC::Handle<NamedObject> namedObject = getNamedObject( name );
        if ( !namedObject->isOperator() )
          throw Exception("");
        operator_ = RC::Handle<Operator>::StaticCast( namedObject );
      }
      catch ( Exception e )
      {
        throw Exception( "no Operator named " + _(name) );
      }
      return operator_;
    }
    
    RC::Handle<Node> Context::getNode( std::string const &name ) const
    {
      RC::Handle<Node> node;
      try
      {
        RC::Handle<Container> container = getContainer( name );
        if ( !container->isNode() )
          throw Exception("");
        node = RC::Handle<Node>::StaticCast( container );
      }
      catch ( Exception e )
      {
        throw Exception( "no Node named " + _(name) );
      }
      return node;
    }
    
    RC::Handle<Event> Context::getEvent( std::string const &name ) const
    {
      RC::Handle<Event> event;
      try
      {
        RC::Handle<Container> container = getContainer( name );
        if ( !container->isEvent() )
          throw Exception("");
        event = RC::Handle<Event>::StaticCast( container );
      }
      catch ( Exception e )
      {
        throw Exception( "no Event named " + _(name) );
      }
      return event;
    }
    
    RC::Handle<EventHandler> Context::getEventHandler( std::string const &name ) const
    {
      RC::Handle<EventHandler> eventHandler;
      try
      {
        RC::Handle<Container> container = getContainer( name );
        if ( !container->isEventHandler() )
          throw Exception("");
        eventHandler = RC::Handle<EventHandler>::StaticCast( container );
      }
      catch ( Exception e )
      {
        throw Exception( "no EventHandler named " + _(name) );
      }
      return eventHandler;
    }
    
    void Context::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      Util::Mutex::Lock mutexLock( m_mutex );
      if ( dst.size() - dstOffset == 0 )
      {
        jsonExec( cmd, arg, resultArrayEncoder );
      }
      else
      {
        JSON::Entity const &first = dst[dstOffset];
        if ( first.stringIs( "DG", 2 ) )
          jsonRouteDG( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
        else if ( first.stringIs( "RT", 2 ) )
          m_rtManager->jsonRoute( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
        else if ( first.stringIs( "IO", 2 ) )
          m_ioManager->jsonRoute( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
        else if ( first.stringIs( "MR", 2 ) )
        {
          MT::TLSLogCollectorAutoSet logCollector( m_logCollector );
          m_mrInterface.jsonRoute( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
        }
        else if ( first.stringIs( "KLC", 3 ) )
        {
          MT::TLSLogCollectorAutoSet logCollector( m_logCollector );
          m_klcInterface.jsonRoute( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
        }
        else throw Exception( "unroutable" );
      }
    }

    void Context::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "getMemoryUsage", 14 ) )
        jsonExecGetMemoryUsage( resultArrayEncoder );
      else throw Exception( "unknown command" );
    }
    
    static void jsonDescLicenses_llvm_projects_sample_autoconf( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::llvm::projects::sample::autoconf::filename );
      memberEncoder.makeString( ThirdPartyLicenses::llvm::projects::sample::autoconf::text );
    }
    
    static void jsonDescLicenses_llvm_projects_sample( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "autoconf", 8 );
      jsonDescLicenses_llvm_projects_sample_autoconf( memberEncoder );
    }
    
    static void jsonDescLicenses_llvm_projects( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "sample", 6 );
      jsonDescLicenses_llvm_projects_sample( memberEncoder );
    }
    
    static void jsonDescLicenses_llvm_lib_Support( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::llvm::lib::Support::filename );
      memberEncoder.makeString( ThirdPartyLicenses::llvm::lib::Support::text );
    }
    
    static void jsonDescLicenses_llvm_lib( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "Support", 7 );
      jsonDescLicenses_llvm_lib_Support( memberEncoder );
    }
    
    static void jsonDescLicenses_llvm_autoconf( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::llvm::autoconf::filename );
      memberEncoder.makeString( ThirdPartyLicenses::llvm::autoconf::text );
    }
    
    static void jsonDescLicenses_llvm( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::llvm::filename );
        memberEncoder.makeString( ThirdPartyLicenses::llvm::text );
      }
      {
        JSON::Encoder libEncoder = resultObjectEncoder.makeMember( "lib", 3 );
        jsonDescLicenses_llvm_lib( libEncoder );
      }
      {
        JSON::Encoder projectsEncoder = resultObjectEncoder.makeMember( "projects", 8 );
        jsonDescLicenses_llvm_projects( projectsEncoder );
      }
      {
        JSON::Encoder autoconfEncoder = resultObjectEncoder.makeMember( "autoconf", 8 );
        jsonDescLicenses_llvm_autoconf( autoconfEncoder );
      }
    }
    
    static void jsonDescLicenses_libpng( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::libpng::filename );
      memberEncoder.makeString( ThirdPartyLicenses::libpng::text );
    }
    
    static void jsonDescLicenses_md5( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::md5::filename );
      memberEncoder.makeString( ThirdPartyLicenses::md5::text );
    }

    static void jsonDescLicenses_liblas( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::liblas::filename );
      memberEncoder.makeString( ThirdPartyLicenses::liblas::text );
    }

    static void jsonDescLicenses_teem( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( ThirdPartyLicenses::teem::filename );
      memberEncoder.makeString( ThirdPartyLicenses::teem::text );
    }

    static void jsonDescLicenses( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "llvm", 4 );
        jsonDescLicenses_llvm( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "md5", 3 );
        jsonDescLicenses_md5( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "libpng", 6 );
        jsonDescLicenses_libpng( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "liblas", 6 );
        jsonDescLicenses_liblas( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "teem", 4 );
        jsonDescLicenses_teem( memberEncoder );
      }
    }

    static void jsonDescBuild( JSON::Encoder &resultEncoder )
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "name", 4 );
        memberEncoder.makeString( buildName );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "pureVersion", 11 );
        memberEncoder.makeString( buildPureVersion );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "fullVersion", 11 );
        memberEncoder.makeString( buildFullVersion );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "desc", 4 );
        memberEncoder.makeString( buildDesc );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "copyright", 9 );
        memberEncoder.makeString( buildCopyright );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "url", 3 );
        memberEncoder.makeString( buildURL );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "os", 2 );
        memberEncoder.makeString( buildOS );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "arch", 4 );
        memberEncoder.makeString( buildArch );
      }
    }

    void Context::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      jsonDesc( resultObjectEncoder );
    }
    
    void Context::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "contextID", 9 );
        memberEncoder.makeString( getContextID() );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "DG", 2 );
        jsonDescDG( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "RT", 2 );
        m_rtManager->jsonDesc( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "EX", 2 );
        Plug::Manager::Instance()->jsonDesc( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "licenses", 8 );
        jsonDescLicenses( memberEncoder );
      }
      {
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( "build", 5 );
        jsonDescBuild( memberEncoder );
      }
    }
    
    void Context::jsonExecGetMemoryUsage( JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      JSON::Encoder elementEncoder = resultArrayEncoder.makeElement();
      JSON::ObjectEncoder elementObjectEncoder = elementEncoder.makeObject();
      JSON::Encoder dgEncoder = elementObjectEncoder.makeMember( "DG" );
      jsonDGGetMemoryUsage( dgEncoder );
    }
    
    void Context::jsonDGGetMemoryUsage( JSON::Encoder &encoder ) const
    {
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      for ( NamedObjectMap::const_iterator it = m_namedObjectRegistry.begin(); it != m_namedObjectRegistry.end(); ++it )
      {
        JSON::Encoder namedEncoder = objectEncoder.makeMember( it->first );
        it->second->jsonGetMemoryUsage( namedEncoder );
      }
    }

    void Context::jsonRouteDG(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 0 )
        jsonExecDG( cmd, arg, resultArrayEncoder );
      else
      {
        std::string namedObjectName = dst[1].stringToStdString();
        NamedObjectMap::const_iterator it = m_namedObjectRegistry.find( namedObjectName );
        if ( it == m_namedObjectRegistry.end() )
          throw Exception( "NamedObject "+_(namedObjectName)+" not found" );
        it->second->jsonRoute( dst, dstOffset + 1, cmd, arg, resultArrayEncoder );
      }
    }

    void Context::jsonExecDG(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "createOperator", 14 ) )
        Operator::jsonExecCreate( arg, this, resultArrayEncoder );
      else if ( cmd.stringIs( "createNode", 10 ) )
        Node::jsonExecCreate( arg, this, resultArrayEncoder );
      else if ( cmd.stringIs( "createResourceLoadNode", 22 ) )
        ResourceLoadNode::jsonExecCreate( arg, this, resultArrayEncoder );
      else if ( cmd.stringIs( "createEvent", 11 ) )
        Event::jsonExecCreate( arg, this, resultArrayEncoder );
      else if ( cmd.stringIs( "createEventHandler", 18 ) )
        EventHandler::jsonExecCreate( arg, this, resultArrayEncoder );
      else throw Exception( "unknown command" );
    }

    void Context::jsonDescDG( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( NamedObjectMap::const_iterator it=m_namedObjectRegistry.begin(); it!=m_namedObjectRegistry.end(); ++it )
      {
        std::string const &namedObjectName = it->first;
        RC::Handle<NamedObject> const &namedObject = it->second;
        {
          JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( namedObjectName );
          namedObject->jsonDesc( memberEncoder );
        }
      }
    }
    
    std::string const Context::s_wrapFabricClientJSSource(
#include <Fabric/Core/DG/FABRIC.Wrappers.js.inc>
);

    static void throwException( size_t length, char const *data )
    {
      throw Exception( length, data );
    }

    RC::ConstHandle<Context> GetAndValidateCurrentContext()
    {
      RC::ConstHandle<Context> context = ExecutionEngine::GetCurrentContext();
      if( !context )
        throw Exception( "Unexpected: undefined context" );
      return context;
    }

    void FileHandleCreateFromPath( void *stringData, char const *filePathCString, bool folder, bool readOnly )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      std::string handle = context->getIOManager()->getFileHandleManager()->createHandle( filePathCString, folder, readOnly );
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      stringDesc->setValue( handle.data(), handle.length(), stringData );
    }

    void FileGetPath( void const *stringData, void *pathStringData )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      std::string handle( stringDesc->getValueData( stringData ), stringDesc->getValueLength( stringData ) );
      std::string path = context->getIOManager()->getFileHandleManager()->getPath( handle );
      stringDesc->setValue( path.data(), path.length(), pathStringData );
    }

    bool FileHandleIsValid( void const *stringData )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      std::string handle( stringDesc->getValueData( stringData ), stringDesc->getValueLength( stringData ) );
      return context->getIOManager()->getFileHandleManager()->isValid( handle );
    }

    bool FileHandleIsReadOnly( void const *stringData )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      std::string handle( stringDesc->getValueData( stringData ), stringDesc->getValueLength( stringData ) );
      return context->getIOManager()->getFileHandleManager()->isReadOnly( handle );
    }

    bool FileHandleIsFolder( void const *stringData )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      std::string handle( stringDesc->getValueData( stringData ), stringDesc->getValueLength( stringData ) );
      return context->getIOManager()->getFileHandleManager()->isFolder( handle );
    }

    bool FileHandleTargetExists( void const *stringData )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      std::string handle( stringDesc->getValueData( stringData ), stringDesc->getValueLength( stringData ) );
      return context->getIOManager()->getFileHandleManager()->targetExists( handle );
    }

    void FileHandleEnsureTargetExists( void const *stringData )
    {
      RC::ConstHandle<Context> context = GetAndValidateCurrentContext();
      RC::ConstHandle<RT::StringDesc> stringDesc = context->getRTManager()->getStringDesc();
      std::string handle( stringDesc->getValueData( stringData ), stringDesc->getValueLength( stringData ) );
      context->getIOManager()->getFileHandleManager()->ensureTargetExists( handle );
    }

    EDK::Callbacks Context::GetCallbackStruct()
    {
      Fabric::EDK::Callbacks callbacks;
      callbacks.m_malloc = malloc;
      callbacks.m_realloc = realloc;
      callbacks.m_free = free;
      callbacks.m_throwException = throwException;
      callbacks.m_fileHandleCreateFromPath = FileHandleCreateFromPath;
      callbacks.m_fileGetPath = FileGetPath;
      callbacks.m_fileHandleIsValid = FileHandleIsValid;
      callbacks.m_fileHandleIsReadOnly = FileHandleIsReadOnly;
      callbacks.m_fileHandleIsFolder = FileHandleIsFolder;
      callbacks.m_fileHandleTargetExists = FileHandleTargetExists;
      callbacks.m_fileHandleEnsureTargetExists = FileHandleEnsureTargetExists;

      return callbacks;
    }
  };
};
