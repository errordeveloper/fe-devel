/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_STRUCT_DECL_MEMBER_VECTOR_H
#define _FABRIC_AST_STRUCT_DECL_MEMBER_VECTOR_H

#include <Fabric/Base/RC/Vector.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/RT/StructMemberInfo.h>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace RT
  {
    class Manager;
  };
  
  namespace CG
  {
    class Manager;
    class ModuleBuilder;
    class Diagnostics;
  };
  
  namespace AST
  {
    class MemberDecl;
    
    class MemberDeclVector : public RC::Vector< RC::ConstHandle<MemberDecl> >
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<MemberDeclVector> Create( RC::ConstHandle<MemberDecl> const &first = 0, RC::ConstHandle<MemberDeclVector> const &remaining = 0 );

      void appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const;
      
      void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
          
      void buildStructMemberInfoVector( RC::ConstHandle<RT::Manager> const &rtManager, RT::StructMemberInfoVector &structMemberInfoVector ) const;
    
    protected:
    
      MemberDeclVector();
    };
  };
};

#endif //_FABRIC_AST_STRUCT_DECL_MEMBER_VECTOR_H
