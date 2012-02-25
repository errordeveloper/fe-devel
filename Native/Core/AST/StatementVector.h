/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_STATEMENT_VECTOR_H
#define _FABRIC_AST_STATEMENT_VECTOR_H

#include <Fabric/Base/RC/Vector.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace CG
  {
    class BasicBlockBuilder;
    class Diagnostics;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class Statement;
    
    class StatementVector : public RC::Vector< RC::ConstHandle<Statement> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<StatementVector> Create( RC::ConstHandle<Statement> const &first = 0, RC::ConstHandle<StatementVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
    
      void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;

    protected:
    
      StatementVector();
    };
  };
};

#endif //_FABRIC_AST_STATEMENT_VECTOR_H
