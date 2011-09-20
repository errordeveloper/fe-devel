/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_AST_GLOBAL_LIST_H
#define _FABRIC_AST_GLOBAL_LIST_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace Util
  {
    class JSONArrayGenerator;
  };
  
  namespace CG
  {
    class Diagnostics;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class Function;
    class Global;
    class UseNameToLocationMap;
    
    class GlobalList : public RC::Object
    {
    public:
      
      static RC::ConstHandle<GlobalList> Create( RC::ConstHandle<Global> const &first = 0, RC::ConstHandle<GlobalList> const &remaining = 0 );
      static RC::ConstHandle<GlobalList> Create( RC::ConstHandle<GlobalList> const &lhs, RC::ConstHandle<GlobalList> const &rhs );

      Util::SimpleString toJSON( bool includeLocation ) const;

      void collectUses( UseNameToLocationMap &uses ) const;
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctions ) const;
      void collectFunctions( std::vector< RC::ConstHandle<AST::Function> > &result ) const;
    
    protected:
    
      GlobalList( RC::ConstHandle<GlobalList> const &before, RC::ConstHandle<Global> const &global, RC::ConstHandle<GlobalList> const &after );
    
      void appendJSON( Util::JSONArrayGenerator const &jsonArrayGenerator, bool includeLocation ) const;
      
    private:
    
      RC::ConstHandle<GlobalList> m_before;
      RC::ConstHandle<Global> m_global;
      RC::ConstHandle<GlobalList> m_after;
    };
  };
};

#endif //_FABRIC_AST_GLOBAL_LIST_H
