/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "Desc.h"
#include "Impl.h"
#include "FixedArrayDesc.h"
#include "VariableArrayDesc.h"

#include <Fabric/Base/JSON/Integer.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Core/Util/Encoder.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Core/Util/JSONGenerator.h>

namespace Fabric
{
  namespace RT
  {
    Desc::Desc( std::string const &userName, RC::ConstHandle<Impl> const &impl )
      : m_userName( userName )
      , m_impl( impl )
    {
    }
    
    size_t Desc::getAllocSize() const
    {
      return m_impl->getAllocSize();
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
    
    bool Desc::equalsData( void const *lhs, void const *rhs ) const
    {
      return m_impl->equalsData( lhs, rhs );
    }
    
    void Desc::disposeData( void *data ) const
    {
      m_impl->disposeData( data );
    }
    
    void Desc::disposeDatas( void *data, size_t count, size_t stride ) const
    {
      m_impl->disposeDatas( data, count, stride );
    }

    std::string Desc::descData( void const *data ) const
    {
      return m_userName + ":" + m_impl->descData( data );
    }

    std::string Desc::toString( void const *data ) const
    {
      return m_impl->descData( data );
    }
    
    RC::Handle<JSON::Value> Desc::getJSONValue( void const *data ) const
    {
      return m_impl->getJSONValue( data );
    }
    
    void Desc::generateJSON( void const *data, Util::JSONGenerator &jsonGenerator ) const
    {
      return m_impl->generateJSON( data, jsonGenerator );
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
      return m_userName == desc->m_userName
        && m_impl->isEquivalentTo( desc->m_impl );
    }
    
    bool Desc::isShallow() const
    {
      return m_impl->isShallow();
    }

    void Desc::jsonDesc( Util::JSONGenerator &resultJG ) const
    {
      Util::JSONObjectGenerator resultJOG = resultJG.makeObject();
      jsonDesc( resultJOG );
    }
    
    void Desc::jsonDesc( Util::JSONObjectGenerator &resultJOG ) const
    {
      resultJOG.makeMember( "name" ).makeString( getUserName() );
      resultJOG.makeMember( "size" ).makeInteger( getAllocSize() );
      Util::JSONGenerator defaultValueJG = resultJOG.makeMember( "defaultValue" );
      generateJSON( getDefaultData(), defaultValueJG );
    }
  };
};
