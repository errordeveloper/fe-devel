/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_PARAM_H
#define _FABRIC_AST_PARAM_H

#include <Fabric/Core/AST/Node.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Scope.h>

namespace llvm
{
  class Value;
}

namespace Fabric
{
  namespace AST
  {
    class Param: public Node
    {
      FABRIC_AST_NODE_DECL( Param );

    public:
    
      static RC::Handle<Param> Create(
        CG::Location const &location,
        std::string const &name,
        std::string const &type,
        CG::Usage usage
        );

      RC::Handle<JSON::Object> toJSON() const;
    
      std::string const &getName() const
      {
        return m_name;
      }
      
      std::string const &getType() const
      {
        return m_type;
      }
      
      CG::Usage getUsage() const
      {
        return m_usage;
      }
      
      CG::FunctionParam getFunctionParam( RC::Handle<CG::Manager> const &cgManager ) const;
      RC::ConstHandle<CG::Adapter> getAdapter( RC::Handle<CG::Manager> const &cgManager ) const;
      CG::ExprType getExprType( RC::Handle<CG::Manager> const &cgManager ) const;
      
      void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics ) const;

    protected:
    
      Param(
        CG::Location const &location,
        std::string const &name,
        std::string const &type,
        CG::Usage usage
        );
      
    private:
    
      std::string m_name;
      std::string m_type;
      CG::Usage m_usage;
    };
  };
};

#endif //_FABRIC_AST_PARAM_H
