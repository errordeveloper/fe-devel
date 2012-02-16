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
  namespace JSON
  {
    class Encoder;
    class ArrayEncoder;
  };
  
  namespace CG
  {
    class Diagnostics;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class Destructor;
    class FunctionBase;
    class Global;
    class UseNameToLocationMap;
    
    class GlobalList : public RC::Object
    {
    public:
      
      static RC::ConstHandle<GlobalList> Create( RC::ConstHandle<Global> const &first = 0, RC::ConstHandle<GlobalList> const &remaining = 0 );
      static RC::ConstHandle<GlobalList> Create( RC::ConstHandle<GlobalList> const &lhs, RC::ConstHandle<GlobalList> const &rhs );

      Util::SimpleString toJSON( bool includeLocation ) const;
      void encodeJSON( bool includeLocation, JSON::Encoder &jg ) const;

      void collectUses( UseNameToLocationMap &uses ) const;
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctions ) const;
      void collectFunctionBases( std::vector< RC::ConstHandle<AST::FunctionBase> > &result ) const;
    
    protected:
    
      GlobalList( RC::ConstHandle<GlobalList> const &before, RC::ConstHandle<Global> const &global, RC::ConstHandle<GlobalList> const &after );
    
      void appendJSON( JSON::ArrayEncoder const &jsonArrayEncoder, bool includeLocation ) const;
      
    private:
    
      RC::ConstHandle<GlobalList> m_before;
      RC::ConstHandle<Global> m_global;
      RC::ConstHandle<GlobalList> m_after;
    };
  };
};

#endif //_FABRIC_AST_GLOBAL_LIST_H
