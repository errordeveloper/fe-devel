/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/Desc.h>

#include <Fabric/Core/RT/FixedArrayDesc.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/VariableArrayDesc.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    RC::ConstHandle<Desc> Desc::Create(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<Impl> const &impl
      )
    {
      return new Desc(
        userNameBase,
        userNameArraySuffix,
        impl
        );
    }
    
    Desc::Desc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<Impl> const &impl
      )
      : m_impl( impl )
      , m_userNameBase( userNameBase )
      , m_userNameArraySuffix( userNameArraySuffix )
      , m_userName( userNameBase + userNameArraySuffix )
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
    
    void Desc::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      m_impl->encodeJSON( data, encoder );
    }
    
    void Desc::decodeJSON( JSON::Entity const &entity, void *data ) const
    {
      m_impl->decodeJSON( entity, data );
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

    bool Desc::isNoAliasSafe() const
    {
      return m_impl->isNoAliasSafe();
    }

    bool Desc::isExportable() const
    {
      return m_impl->isExportable();
    }
    
    void Desc::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      jsonDesc( resultObjectEncoder );
    }
    
    void Desc::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      resultObjectEncoder.makeMember( "name" ).makeString( getUserName() );
      resultObjectEncoder.makeMember( "size" ).makeInteger( getAllocSize() );
      JSON::Encoder defaultValueObjectEncoder = resultObjectEncoder.makeMember( "defaultValue" );
      encodeJSON( getDefaultData(), defaultValueObjectEncoder );
    }

    size_t Desc::getIndirectMemoryUsage( void const *data ) const
    {
      return m_impl->getIndirectMemoryUsage( data );
    }
  }
}
