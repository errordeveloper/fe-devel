/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_BIN_OP_IMPL_H
#define _FABRIC_AST_BIN_OP_IMPL_H

#include <Fabric/Core/AST/FunctionBase.h>
#include <Fabric/Core/CG/OpTypes.h>

namespace llvm
{
  class Module;
  class FunctionPassManager;
};

namespace Fabric
{
  namespace RT
  {
  };
  
  namespace AST
  {
    class CompoundStatement;
    class Param;
    
    class BinOpImpl : public FunctionBase
    {
      FABRIC_AST_NODE_DECL( BinOpImpl );
      
    public:
    
      static RC::ConstHandle<BinOpImpl> Create(
        CG::Location const &location,
        std::string const &returnType,
        CG::BinOpType binOpType,
        RC::ConstHandle<Param> const &lhs,
        RC::ConstHandle<Param> const &rhs,
        RC::ConstHandle<CompoundStatement> const &body
        );
                  
      virtual std::string getEntryName( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual RC::ConstHandle<ParamVector> getParams( RC::Handle<CG::Manager> const &cgManager ) const;
      
    protected:
    
      BinOpImpl(
        CG::Location const &location,
        std::string const &returnType,
        CG::BinOpType binOpType,
        RC::ConstHandle<Param> const &lhs,
        RC::ConstHandle<Param> const &rhs,
        RC::ConstHandle<CompoundStatement> const &body
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
        
    private:
    
      CG::BinOpType m_binOpType;
      RC::ConstHandle<ParamVector> m_params;
    };
  };
};

#endif //_FABRIC_AST_BIN_OP_IMPL_H
