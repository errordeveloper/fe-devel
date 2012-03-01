/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PLUG_INST_H
#define _FABRIC_PLUG_INST_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Core/Plug/Desc.h>
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/Plug/Helpers.h>
#include <Fabric/Core/Util/AutoPtr.h>

namespace Fabric
{
  namespace JSON
  {
    class CommandChannel;
  };

  namespace RT
  {
    class Impl;
    class Manager;
  };

  namespace EDK
  {
    struct Callbacks;
  };

  namespace CG
  {
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class GlobalList;
  };

  namespace DG
  {
    class Context;
  };
  
  namespace LIB
  {
    class Value;
    class Object;
  };
  
  namespace IO
  {
    class Dir;
  };
  
  namespace Plug
  {
    class Inst : public RC::Object
    {
      typedef std::map< std::string, SOLibHandle > ResolvedNameToSOLibHandleMap;
      
      typedef std::map< std::string, void * > ExternalFunctionMap;
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Inst> Create(
        RC::ConstHandle<IO::Dir> const &extensionDir,
        std::string const &extensionName,
        std::string const &jsonDesc,
        std::vector<std::string> const &pluginDirs,
        RC::Handle<CG::Manager> const &cgManager,
        EDK::Callbacks const &callbacks,
        std::map< std::string, void (*)( void * ) > &implToDestructorMap
        );
      
      std::string const &getJSONDesc() const
      {
        return m_jsonDesc;
      }
      
      Desc const &getDesc() const
      {
        return m_desc;
      }
      
      std::string const &getCode() const
      {
        return m_code;
      }
      
      RC::ConstHandle<AST::GlobalList> getAST() const;
      RC::ConstHandle<AST::GlobalList> maybeGetASTForExt( std::string const &extName ) const;
      void *llvmResolveExternalFunction( std::string const &name ) const;

      virtual void jsonDesc( JSON::Encoder &resultEncoder ) const;
      
    protected:
    
      Inst(
        RC::ConstHandle<IO::Dir> const &extensionDir,
        std::string const &extensionName,
        std::string const &jsonDesc,
        std::vector<std::string> const &pluginDirs,
        RC::Handle<CG::Manager> const &cgManager,
        EDK::Callbacks const &callbacks,
        std::map< std::string, void (*)( void * ) > &implToDestructorMap
        );
      ~Inst();
      
    private:
    
      bool hasMethod( std::string const &methodName ) const;
      void enumerateMethods( std::vector<std::string> &result ) const;
      //RC::Handle<LIB::Value> invokeMethod( std::string const &methodName, std::vector< RC::Handle<LIB::Value> > const &args );
    
      std::string m_name;
      RC::Handle<CG::Manager> m_cgManager;
      std::string m_jsonDesc;
      Desc m_desc;
      std::string m_code;
      RC::ConstHandle<AST::GlobalList> m_ast;
      CG::Diagnostics m_diagnostics;
      ResolvedNameToSOLibHandleMap m_resolvedNameToSOLibHandleMap;
      std::vector<SOLibHandle> m_orderedSOLibHandles;
      ExternalFunctionMap m_externalFunctionMap;
      //RC::Handle<LIB::Object> m_fabricLIBObject;
      //MethodMap m_methodMap;
      std::string m_jsConstants;
    };
  };
};

#endif //_FABRIC_PLUG_INST_H
