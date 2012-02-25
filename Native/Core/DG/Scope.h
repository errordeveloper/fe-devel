/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_SCOPE_H
#define _FABRIC_DG_SCOPE_H

#include <Fabric/Core/DG/Node.h>
#include <Fabric/Base/RC/WeakHandle.h>

namespace Fabric
{
  namespace DG
  {
    class Scope
    {
    public:
    
      Scope( Scope const *parentScope = 0 )
        : m_parentScope( parentScope )
      {
      }
      
      virtual RC::Handle<Container> find( std::string const &name ) const
      {
        if ( m_parentScope )
          return m_parentScope->find( name );
        else return 0;
      }
      
      virtual std::string desc() const
      {
        if ( m_parentScope )
          return "->" + m_parentScope->desc();
        else return "";
      }
      
    private:
    
      Scope const *m_parentScope;
    };
    
    class BindingsScope : public Scope
    {
    public:
    
      typedef Util::UnorderedMap< std::string, RC::Handle<Node> > Bindings;

      BindingsScope( Bindings const &bindings, Scope const *parentScope = 0 )
        : Scope( parentScope )
        , m_bindings( bindings )
      {
      }
      
      RC::Handle<Container> find( std::string const &name ) const
      {
        Bindings::const_iterator it = m_bindings.find( name );
        if ( it != m_bindings.end() )
          return it->second;
        else return Scope::find( name );
      }
      
      virtual std::string desc() const
      {
        std::string result = "[";
        for ( Bindings::const_iterator it=m_bindings.begin(); it!=m_bindings.end(); ++it )
        {
          if ( result.length() > 1 )
            result += ",";
          result += it->first;
        }
        return result + "]" + Scope::desc();
      }
      
    private:
    
      Bindings const &m_bindings;
    };
    
    class NamedScope : public Scope
    {
    public:
   
      NamedScope( std::string const &name, RC::Handle<Container> const &container, Scope const *parentScope = 0 )
        : Scope( parentScope )
        , m_name( name )
        , m_container( container )
      {
      }
      
      RC::Handle<Container> find( std::string const &name ) const
      {
        if ( name == m_name )
          return m_container.makeStrong();
        else return Scope::find( name );
      }
      
      virtual std::string desc() const
      {
        return "[" + m_name + "]" + Scope::desc();
      }
      
    private:
    
      std::string m_name;
      RC::WeakHandle<Container> m_container;
    };
    
    class SelfScope : public NamedScope
    {
    public:
    
      SelfScope( RC::Handle<Container> const &self, Scope const *parentScope = 0 )
        : NamedScope( "self", self, parentScope )
      {
      }
    };
  };
};

#endif //_FABRIC_DG_SCOPE_H
