/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_GLOBAL_H
#define _FABRIC_AST_GLOBAL_H

#include <Fabric/Core/AST/Node.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace CG
  {
    class Diagnostics;
    class Location;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class RequireNameToLocationMap;

    class Global : public Node
    {
    public:
      REPORT_RC_LEAKS
    
      virtual void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
    
      virtual bool isFunctionBase() const { return false; }
      
      virtual void collectRequires( RequireNameToLocationMap &requires ) const;
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const = 0;
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const = 0;
      
    protected:
    
      Global( CG::Location const &location );
      
    private:
    
      mutable Util::SimpleString m_json;
    };
  }
}

#endif //_FABRIC_AST_GLOBAL_H
