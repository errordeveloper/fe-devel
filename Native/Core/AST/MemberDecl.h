/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_MEMBER_DECL_H
#define _FABRIC_AST_MEMBER_DECL_H

#include <Fabric/Core/AST/Node.h>
#include <Fabric/Core/RT/StructMemberInfo.h>

namespace Fabric
{
  namespace RT
  {
    class Manager;
  };
  
  namespace CG
  {
    class Adapter;
    class Diagnostics;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class MemberDecl : public Node
    {
      FABRIC_AST_NODE_DECL( MemberDecl );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<MemberDecl> Create( CG::Location const &location, std::string const &name, std::string const &typeName )
      {
        return new MemberDecl( location, name, typeName );
      }

      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      void buildStructMemberInfo( RC::ConstHandle<RT::Manager> const &rtManager, RT::StructMemberInfo &structMemberInfo ) const;

    protected:
    
      MemberDecl( CG::Location const &location, std::string const &name, std::string const &typeName );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      std::string m_name;
      std::string m_type;
    };
  };
};

#endif //_FABRIC_AST_MEMBER_DECL_H
