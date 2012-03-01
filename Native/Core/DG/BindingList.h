/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_BINDING_LIST_H
#define _FABRIC_DG_BINDING_LIST_H

#include <Fabric/Core/DG/CompiledObject.h>

#include <list>
#include <string>

namespace Fabric
{
  namespace JSON
  {
    class ArrayDecoder;
    class ArrayEncoder;
    class Encoder;
    struct Entity;
  };
  
  namespace DG
  {
    class Binding;
    class Context;
    class NamedObject;
    
    class BindingList : public CompiledObject
    {
      typedef std::list< RC::Handle<Binding> > Bindings;
      
    public:
      REPORT_RC_LEAKS
      
      static RC::Handle<BindingList> Create( RC::Handle<Context> const &context );
      
      void addOwner( NamedObject *owner, std::string const &subName );
      void removeOwner( NamedObject *owner );
      
      RC::Handle<Binding> get( size_t idx );
      void append( RC::Handle<Binding> const &op );
      void insert( RC::Handle<Binding> const &op, size_t beforeIdx );
      void move( size_t fromIdx, size_t toBeforeIdx );
      void remove( size_t idx );
      void clear();
      size_t size() const;
      
      void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      void jsonExecAppend( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecInsert( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecRemove( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      
    protected:
    
      BindingList( RC::Handle<Context> const &context );
      ~BindingList();
    
      void jsonNotify() const;
      
      virtual void propagateMarkForRecompileImpl( unsigned generation );    
      virtual void propagateMarkForRefreshImpl( unsigned generation );    
      virtual void collectErrors();
      virtual void invalidateRunState();
      virtual void refreshRunState();
      virtual void collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const;
      virtual bool canExecute() const;
      
    private:
      
      Context *m_context;
      Bindings m_bindings;
      std::string m_subName;
      NamedObject *m_owner;
    };
  }
}

#endif // _FABRIC_DG_BINDING_LIST_H
