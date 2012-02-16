/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_AST_FUNCTION_BASE_H
#define _FABRIC_AST_FUNCTION_BASE_H

#include <Fabric/Core/AST/Global.h>
#include <Fabric/Core/AST/CompoundStatement.h>

namespace llvm
{
  class Module;
  class FunctionPassManager;
};

namespace Fabric
{
  namespace CG
  {
    class Adapter;
    class Manager;
  };
  
  namespace AST
  {
    class ParamVector;
    
    class FunctionBase : public Global
    {
    public:
          
      virtual bool isFunctionBase() const { return true; }
      virtual bool isFunction() const { return false; }
      virtual bool isDestructor() const { return false; }
      
      std::string getSymbolName( RC::Handle<CG::Manager> const &cgManager ) const;
      
      virtual std::string const *getScopeName( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const = 0;
      virtual std::string getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const = 0;
      virtual std::string getDesc( RC::Handle<CG::Manager> const &cgManager ) const = 0;
      virtual RC::ConstHandle<ParamVector> getParams( RC::Handle<CG::Manager> const &cgManager ) const = 0;

      std::string const &getReturnTypeName() const
      {
        return m_returnTypeName;
      }
      RC::ConstHandle<CompoundStatement> getBody() const;
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      FunctionBase(
        CG::Location const &location,
        std::string const &returnTypeName,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body,
        bool exportSymbol
        );
      
      RC::ConstHandle<CG::Adapter> getReturnAdapter( RC::Handle<CG::Manager> const &cgManager ) const;
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      std::string m_returnTypeName;
      std::string m_symbolName;
      RC::ConstHandle<CompoundStatement> m_body;
      bool m_exportSymbol;
    };
  };
};

#endif //_FABRIC_AST_FUNCTION_BASE_H
