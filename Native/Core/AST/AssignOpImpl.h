/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_ASSIGN_OP_IMPL_H
#define _FABRIC_AST_ASSIGN_OP_IMPL_H

#include <Fabric/Core/AST/FunctionBase.h>
#include <Fabric/Core/CG/OpTypes.h>

namespace Fabric
{
  namespace AST
  {
    class CompoundStatement;
    class Param;
    
    class AssignOpImpl : public FunctionBase
    {
      FABRIC_AST_NODE_DECL( AssignOpImpl );
      
    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<AssignOpImpl> Create(
        CG::Location const &location,
        std::string const &thisTypeName,
        CG::AssignOpType assignOpType,
        RC::ConstHandle<Param> rhs,
        std::string const *scopeName,
        RC::ConstHandle<CompoundStatement> const &body
        );
                  
      virtual std::string getPencilKey( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getDefaultSymbolName( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual std::string getDesc( RC::Handle<CG::Manager> const &cgManager ) const;
      virtual RC::ConstHandle<ParamVector> getParams( RC::Handle<CG::Manager> const &cgManager ) const;
      
    protected:
    
      AssignOpImpl(
        CG::Location const &location,
        std::string const &thisTypeName,
        CG::AssignOpType assignOpType,
        RC::ConstHandle<Param> rhs,
        std::string const *scopeName,
        RC::ConstHandle<CompoundStatement> const &body
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      std::string m_thisTypeName;
      CG::AssignOpType m_assignOpType;
      RC::ConstHandle<Param> m_rhs;
    };
  }
}

#endif //_FABRIC_AST_ASSIGN_OP_IMPL_H
