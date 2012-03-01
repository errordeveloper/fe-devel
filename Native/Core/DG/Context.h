/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_CONTEXT_H
#define _FABRIC_DG_CONTEXT_H

#include <Fabric/Core/JSON/CommandChannel.h>
#include <Fabric/Core/DG/CompiledObject.h>
#include <Fabric/Core/MR/Interface.h>
#include <Fabric/Core/KLC/Interface.h>
#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Core/Util/UnorderedMap.h>
#include <Fabric/Base/RC/WeakHandleSet.h>
#include <Fabric/Base/Util/AtomicSize.h>

#include <vector>

namespace Fabric
{
  namespace MT
  {
    class LogCollector;
    class IdleTaskQueue;
  };
  
  namespace RT
  {
    class Manager;
  };
  
  namespace CG
  {
    class Manager;
  };
  
  namespace EDK
  {
    struct Callbacks;
  };

  namespace IO
  {
    class Manager;
  };
  
  namespace Plug
  {
    class Manager;
  };
  
  namespace DG
  {
    class NamedObject;
    class Container;
    class Node;
    class Event;
    class EventHandler;
    class Operator;
    class BindingList;
    class CodeManager;
    class Client;
    
    class Context : public JSON::CommandChannel
    {
      friend class CompiledObject; // [pzion 20120223] For access to getCompiledObjectGlobalData()
      
      typedef std::set<Client *> Clients;
      typedef Util::UnorderedMap< std::string, Context * > ContextMap;
      
      void openNotificationBracket();
      void closeNotificationBracket();
      void registerCoreTypes();
    
    public:
      REPORT_RC_LEAKS
    
      class NotificationBracket
      {
      public:
      
        NotificationBracket( RC::Handle<Context> const &context )
          : m_context( context )
        {
          m_context->openNotificationBracket();
        }
        
        ~NotificationBracket()
        {
          m_context->closeNotificationBracket();
        }
        
      private:
      
        RC::Handle<Context> m_context;
      };
    
      typedef Util::UnorderedMap< std::string, RC::Handle<NamedObject> > NamedObjectMap;
    
      static RC::Handle<Context> Create(
        RC::Handle<IO::Manager> const &ioManager,
        std::vector<std::string> const &pluginDirs,
        CG::CompileOptions const &compileOptions,
        bool optimizeSynchronously
        );
      static RC::Handle<Context> Bind( std::string const &contextID );
      
      std::string const &getContextID() const;

      RC::Handle<MT::LogCollector> getLogCollector() const;
      RC::Handle<RT::Manager> getRTManager() const;
      RC::Handle<CG::Manager> getCGManager() const;
      RC::Handle<IO::Manager> getIOManager() const;
      RC::Handle<CodeManager> getCodeManager() const;
      
      NamedObjectMap &getNamedObjectRegistry() const;
      
      RC::Handle<NamedObject> getNamedObject( std::string const &name ) const;
      RC::Handle<Container> getContainer( std::string const &name ) const;
      RC::Handle<Operator> getOperator( std::string const &name ) const;
      RC::Handle<Node> getNode( std::string const &name ) const;
      RC::Handle<Event> getEvent( std::string const &name ) const;
      RC::Handle<EventHandler> getEventHandler( std::string const &name ) const;
    
      virtual void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      virtual void jsonDesc( JSON::Encoder &resultEncoder ) const;
      
      void jsonRouteDG( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecDG( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDescDG( JSON::Encoder &resultEncoder ) const;

      void registerClient( Client *client );
      virtual void jsonNotify( std::vector<std::string> const &srcs, char const *cmdData, size_t cmdLength, Util::SimpleString const *arg = 0 );
      void unregisterClient( Client *client );
      
      static std::string const &GetWrapFabricClientJSSource()
      {
        return s_wrapFabricClientJSSource;
      }
      
      static EDK::Callbacks GetCallbackStruct();

      void acquireMutex()
      {
        m_mutex.acquire();
      }
      void releaseMutex()
      {
        m_mutex.release();
      }
      
    protected:
    
      Context(
        RC::Handle<IO::Manager> const &ioManager,
        std::vector<std::string> const &pluginDirs,
        CG::CompileOptions const &compileOptions,
        bool optimizeSynchronously
        );
      ~Context();

      void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      void jsonExecGetMemoryUsage( JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonDGGetMemoryUsage( JSON::Encoder &jg ) const;
      
      CompiledObject::GlobalData *getCompiledObjectGlobalData()
      {
        return &m_compiledObjectGlobalData;
      }

    private:
    
      static Util::Mutex s_contextMapMutex;
      static ContextMap s_contextMap;
      std::string m_contextID;
    
      Util::Mutex m_mutex;
      RC::Handle<MT::LogCollector> m_logCollector;
      RC::Handle<RT::Manager> m_rtManager;
      RC::Handle<IO::Manager> m_ioManager;
      RC::Handle<CG::Manager> m_cgManager;
      CG::CompileOptions m_compileOptions;
      RC::Handle<CodeManager> m_codeManager;
      
      mutable NamedObjectMap m_namedObjectRegistry;
      
      Util::Mutex m_clientsMutex;
      Clients m_clients;
      
      Util::AtomicSize m_notificationBracketCount;
      Util::Mutex m_pendingNotificationsMutex;
      Util::SimpleString *m_pendingNotificationsJSON;
      JSON::Encoder *m_pendingNotificationsEncoder;
      JSON::ArrayEncoder *m_pendingNotificationsArrayEncoder;
      
      static std::string const s_wrapFabricClientJSSource;
      
      GC::Container m_gcContainer;
      MR::Interface m_mrInterface;
      KLC::Interface m_klcInterface;
      
      CompiledObject::GlobalData m_compiledObjectGlobalData;
    };
  }
}

#endif //_FABRIC_DG_CONTEXT_H
