/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
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
    class Array;
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
      
      static RC::Handle<MemberDeclVector> Create();
      static RC::Handle<MemberDeclVector> Create( RC::ConstHandle<MemberDecl> const &first );
      static RC::Handle<MemberDeclVector> Create( RC::ConstHandle<MemberDecl> const &first, RC::Handle<MemberDeclVector> const &remaining );

      RC::Handle<JSON::Array> toJSON() const;
          
      void buildStructMemberInfoVector( RC::ConstHandle<RT::Manager> const &rtManager, RT::StructMemberInfoVector &structMemberInfoVector ) const;
      void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics ) const;
    
    protected:
    
      MemberDeclVector();
    };
  };
};

#endif //_FABRIC_AST_STRUCT_DECL_MEMBER_VECTOR_H
