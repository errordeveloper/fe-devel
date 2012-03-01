/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_METHOD_OP_IMPL_H
#define _FABRIC_AST_METHOD_OP_IMPL_H

#include <Fabric/Core/AST/FunctionBase.h>
#include <Fabric/Core/CG/ExprType.h>

namespace llvm
{
  class Module;
  class FunctionPassManager;
};

namespace Fabric
{
  namespace AST
  {
    class MethodOpImpl : public FunctionBase
    {
      FABRIC_AST_NODE_DECL( MethodOpImpl );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<MethodOpImpl> Create(
        CG::Location const &location,
        std::string const &returnTypeName,
        std::string const &thisTypeName,
        std::string const &methodName,
        RC::ConstHandle<ParamVector> const &params,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body
        );
          
      virtual std::string getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getDesc( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual RC::ConstHandle<ParamVector> getParams( RC::Handle<CG::Manager> const &cgManager ) const;
              
    protected:
    
      MethodOpImpl(
        CG::Location const &location,
        std::string const &returnTypeName,
        std::string const &thisTypeName,
        std::string const &methodName,
        RC::ConstHandle<ParamVector> const &params,
        std::string const *symbolName,
        RC::ConstHandle<CompoundStatement> const &body
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
        
    private:
    
      std::string m_thisTypeName;
      CG::Usage m_thisUsage;
      std::string m_methodName;
      RC::ConstHandle<ParamVector> m_params;
    };
  }
}

#endif //_FABRIC_AST_METHOD_OP_IMPL_H
