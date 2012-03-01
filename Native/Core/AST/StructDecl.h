/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_STRUCT_DECL_H
#define _FABRIC_AST_STRUCT_DECL_H

#include <Fabric/Core/AST/Global.h>

namespace Fabric
{
  namespace JSON
  {
    class ObjectEncoder;
  };
  
  namespace RT
  {
    class Manager;
  };
  
  namespace CG
  {
    class Adapter;
    class Diagnostics;
    class Location;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class MemberDeclVector;
    
    class StructDecl : public Global
    {
      FABRIC_AST_NODE_DECL( StructDecl );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<StructDecl> Create(
        CG::Location const &location,
        std::string const &name,
        RC::ConstHandle<MemberDeclVector> const &members
        );

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      StructDecl(
        CG::Location const &location,
        std::string const &name,
        RC::ConstHandle<MemberDeclVector> const &members
        );
      
      void registerTypes( RC::Handle<RT::Manager> const &rtManager, CG::Diagnostics &diagnostics ) const;
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      std::string m_name;
      RC::ConstHandle<MemberDeclVector> m_members;
    };
  };
};

#endif //_FABRIC_AST_STRUCT_DECL_H
