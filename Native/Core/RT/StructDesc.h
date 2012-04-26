/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_STRUCT_DESC_H
#define _FABRIC_RT_STRUCT_DESC_H

#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/StructMemberInfo.h>

namespace Fabric
{
  namespace AST
  {
    class StructDecl;
  }

  namespace RT
  {
    class StructImpl;
    
    class StructDesc : public Desc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
          
      size_t getNumMembers() const;
      StructMemberInfo const &getMemberInfo( size_t index ) const;
      
      void const *getImmutableMemberData( void const *data, size_t index ) const;
      void *getMutableMemberData( void *data, size_t index ) const;
     
      bool hasMember( std::string const &name ) const;
      size_t getMemberIndex( std::string const &name ) const;
          
      RC::Handle<RC::Object> getPrototype() const;
      void setPrototype( RC::Handle<RC::Object> const &prototype ) const;
      
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;

      RC::ConstHandle<AST::StructDecl> getASTStructDecl() const;

    protected:
    
      StructDesc(
        std::string const &userNameBase,
        std::string const &userNameArraySuffix,
        RC::ConstHandle<StructImpl> const &structImpl,
        RC::ConstHandle<AST::StructDecl> const &existingStructDecl
        );
 
    private:
      
      RC::ConstHandle<StructImpl> m_structImpl;
      mutable RC::Handle<RC::Object> m_prototype;

      RC::ConstHandle<AST::StructDecl> m_astStructDecl;
    };
  }
}

#endif //_FABRIC_RT_STRUCT_DESC_H
