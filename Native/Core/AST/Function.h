/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_AST_FUNCTION_H
#define _FABRIC_AST_FUNCTION_H

#include <Fabric/Core/AST/FunctionBase.h>
#include <Fabric/Core/AST/ParamVector.h>

namespace Fabric
{
  namespace AST
  {
    class Function : public FunctionBase
    {
      FABRIC_AST_NODE_DECL( Function );

    public:

      static RC::ConstHandle<Function> Create(
        CG::Location const &location,
        std::string const &returnTypeName,
        std::string const &functionName,
        RC::ConstHandle<ParamVector> const &params,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body
        );
          
      virtual bool isFunction() const { return true; }
      virtual bool isOperator() const { return false; }
      
      std::string const &getDeclaredName() const
      {
        return m_functionName;
      }
      
      virtual std::string const *getScopeName( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getDesc( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual RC::ConstHandle<ParamVector> getParams( RC::Handle<CG::Manager> const &cgManager ) const;
      
    protected:
    
      Function(
        CG::Location const &location,
        std::string const &returnTypeName,
        std::string const &functionName,
        RC::ConstHandle<ParamVector> const &params,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body,
        bool exportSymbol
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      std::string m_functionName;
      RC::ConstHandle<ParamVector> m_params;
    };
  }
}

#endif //_FABRIC_AST_FUNCTION_H
