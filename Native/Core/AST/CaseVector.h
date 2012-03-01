/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CASE_VECTOR_H
#define _FABRIC_AST_CASE_VECTOR_H

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
    class Case;
    
    class CaseVector : public RC::Vector< RC::ConstHandle<Case> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<CaseVector> Create( RC::ConstHandle<Case> const &first = 0, RC::ConstHandle<CaseVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
    
    protected:
    
      CaseVector();
    };
  };
};

#endif //_FABRIC_AST_CASE_VECTOR_H
