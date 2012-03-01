/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_EXPR_VECTOR_H
#define _FABRIC_AST_EXPR_VECTOR_H

#include <Fabric/Base/RC/Vector.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/CG/ExprType.h>

#include <vector>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace CG
  {
    class Adapter;
    class BasicBlockBuilder;
    class ExprValue;
    class Manager;
    class ModuleBuilder;
    class Diagnostics;
  };
  
  namespace AST
  {
    class Expr;
    
    class ExprVector : public RC::Vector< RC::ConstHandle<Expr> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<ExprVector> Create( RC::ConstHandle<Expr> const &first = 0, RC::ConstHandle<ExprVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
          
      void appendAdapters( CG::BasicBlockBuilder &basicBlockBuilder, std::vector< RC::ConstHandle<CG::Adapter> > &adapters ) const;
      void appendExprTypes( CG::BasicBlockBuilder &basicBlockBuilder, std::vector<CG::ExprType> &exprTypes ) const;
      void appendExprValues( CG::BasicBlockBuilder &basicBlockBuilder, std::vector<CG::Usage> const &usages, std::vector<CG::ExprValue> &result, std::string const &lValueErrorDesc ) const;
    
    protected:
    
      ExprVector();
    };
  };
};

#endif //_FABRIC_AST_EXPR_VECTOR_H
