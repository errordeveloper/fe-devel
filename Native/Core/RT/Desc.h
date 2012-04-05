/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_DESC_H
#define _FABRIC_RT_DESC_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Core/RT/ImplType.h>

#include <stdint.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace JSON
  {
    class Decoder;
    class Encoder;
    struct Entity;
    class ObjectEncoder;
  };
  
  namespace RT
  {
    class VariableArrayDesc;
    class FixedArrayDesc;
    class Impl;
  
    class Desc : public RC::Object
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<Desc> Create(
        std::string const &userNameBase,
        std::string const &userNameArraySuffix,
        RC::ConstHandle<Impl> const &impl
        );
    
      std::string const &getUserName() const
      {
        return m_userName;
      }
      size_t getAllocSize() const;
      RC::ConstHandle<Impl> getImpl() const;
      
      ImplType getType() const;
      virtual bool isArrayDesc() const { return false; }
      
      void initializeData( void const *initialData, void *data ) const;
      void initializeDatas( size_t count, void const *initialData, size_t initialStride, void *data, size_t stride ) const;
      void setData( void const *value, void *data ) const;
      void setDatas( size_t count, void const *srcData, size_t srcStride, void *dstData, size_t dstStride ) const;
      void disposeData( void *data ) const;
      void disposeDatas( size_t count, void *data, size_t stride ) const;

      void const *getDefaultData() const;
      std::string descData( void const *data ) const;
      std::string toString( void const *data ) const;
      bool equalsData( void const *lhs, void const *rhs ) const;
      size_t getIndirectMemoryUsage( void const *data ) const;
      
      void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      void decodeJSON( JSON::Entity const &entity, void *data ) const;

      void setKLBindingsAST( RC::ConstHandle<RC::Object> const &klBindingsAST ) const;
      RC::ConstHandle<RC::Object> getKLBindingsAST() const;
      
      bool isEquivalentTo( RC::ConstHandle< RT::Desc > const &desc ) const;
      bool isShallow() const;
      bool isNoAliasSafe() const;
      bool isNoAliasUnsafe() const;
      bool isExportable() const;
      
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      
    protected:
    
      Desc(
        std::string const &userNameBase,
        std::string const &userNameArraySuffix,
        RC::ConstHandle<Impl> const &impl
        );
      
      std::string const &getUserNameBase() const
      {
        return m_userNameBase;
      }
      
      std::string const &getUserNameArraySuffix() const
      {
        return m_userNameArraySuffix;
      }
      
    private:
    
      RC::ConstHandle<Impl> m_impl;
      std::string m_userNameBase;
      std::string m_userNameArraySuffix;
      std::string m_userName;
      
      mutable RC::ConstHandle<RC::Object> m_klBindingsAST;
    };
  }
}

#endif // _FABRIC_RT_DESC_H
