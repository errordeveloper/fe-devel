#include "Desc.h"
#include "Impl.h"
#include "FixedArrayDesc.h"
#include "VariableArrayDesc.h"

#include <Fabric/Base/JSON/Integer.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Core/Util/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  

  namespace RT
  {
    Desc::Desc( std::string const &name, RC::ConstHandle<Impl> const &impl )
      : m_name( name )
      , m_impl( impl )
    {
    }
    
    size_t Desc::getSize() const
    {
      return m_impl->getSize();
    }
    
    RC::ConstHandle<Impl> Desc::getImpl() const
    {
      return m_impl;
    }
    
    ImplType Desc::getType() const
    {
      return m_impl->getType();
    }
    
    void const *Desc::getDefaultData() const
    {
      return m_impl->getDefaultData();
    }
    
    void Desc::setData( void const *srcData, void *dstData ) const
    {
      m_impl->setData( srcData, dstData );
    }
    
    void Desc::disposeData( void *data ) const
    {
      m_impl->disposeData( data );
    }

    std::string Desc::descData( void const *data ) const
    {
      return m_name + ":" + m_impl->descData( data );
    }
    
    RC::Handle<JSON::Value> Desc::getJSONValue( void const *data ) const
    {
      return m_impl->getJSONValue( data );
    }
    
    void Desc::setDataFromJSONValue( RC::ConstHandle<JSON::Value> const &value, void *data ) const
    {
      m_impl->setDataFromJSONValue( value, data );
    }

    void Desc::setKLBindingsAST( RC::ConstHandle<RC::Object> const &klBindingsAST ) const
    {
      m_klBindingsAST = klBindingsAST;
    }
    
    RC::ConstHandle<RC::Object> Desc::getKLBindingsAST() const
    {
      return m_klBindingsAST;
    }

    bool Desc::isEquivalentTo( RC::ConstHandle<Desc> const &desc ) const
    {
      return m_name == desc->m_name
        && m_impl->isEquivalentTo( desc->m_impl );
    }
    
    bool Desc::isShallow() const
    {
      return m_impl->isShallow();
    }

    RC::Handle<JSON::Object> Desc::jsonDesc() const
    {
      RC::Handle<JSON::Object> result = JSON::Object::Create();
      result->set( "name", JSON::String::Create( getName() ) );
      result->set( "size", JSON::Integer::Create( getSize() ) );
      result->set( "defaultValue", getJSONValue( getDefaultData() ) );
      return result;
    }
  };
};
