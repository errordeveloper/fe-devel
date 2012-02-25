/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_VAR_DECL_VECTOR_H
#define _FABRIC_AST_VAR_DECL_VECTOR_H

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
    class VarDecl;
    
    class VarDeclVector : public RC::Vector< RC::ConstHandle<VarDecl> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<VarDeclVector> Create( RC::ConstHandle<VarDecl> const &first = 0, RC::ConstHandle<VarDeclVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      
      void registerTypes( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
    
      void llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;

    protected:
    
      VarDeclVector();
    };
  }
}

#endif //_FABRIC_AST_VAR_DECL_VECTOR_H
