/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_RT_INTEGER_DESC_H
#define _FABRIC_RT_INTEGER_DESC_H

#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/IntegerImpl.h>

namespace Fabric
{
  namespace RT
  {
    class IntegerDesc : public NumericDesc
    {
      friend class Manager;
      
    public:
    
      bool isSigned() const
      {
        return m_integerImpl->isSigned();
      }
      
    protected:
    
      IntegerDesc( std::string const &name, RC::ConstHandle<IntegerImpl> const &integerImpl )
        : NumericDesc( name, integerImpl )
        , m_integerImpl( integerImpl )
      {
      }
      
    private:
    
      RC::ConstHandle<IntegerImpl> m_integerImpl;
    };

    template<typename T> class IntegerDescT : public IntegerDesc
    {
    public:
    
      T getValue( void const *data ) const
      {
        return m_integerImplT->getValue( data );
      }
      
      void setValue( T t, void *data ) const
      {
        m_integerImplT->setValue( t, data );
      }
          
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
      {
        Desc::jsonDesc( resultObjectEncoder );
        std::string internalType;
        if ( m_integerImplT->isSigned() )
          internalType += "si";
        else
          internalType += "ui";
        internalType += _(m_integerImplT->getAllocSize() * 8);
        resultObjectEncoder.makeMember( "internalType" ).makeString( internalType );
      }
      
    protected:
    
      IntegerDescT( std::string const &name, RC::ConstHandle< IntegerImplT<T> > const &integerImplT )
        : IntegerDesc( name, integerImplT )
        , m_integerImplT( integerImplT )
      {
      }
      
    private:
    
      RC::ConstHandle< IntegerImplT<T> > m_integerImplT;
    };
    
    class ByteDesc : public IntegerDescT<uint8_t>
    {
      friend class Manager;
      
    protected:
    
      ByteDesc( std::string const &name, RC::ConstHandle<ByteImpl> const &byteImpl )
        : IntegerDescT<uint8_t>( name, byteImpl )
      {
      }
    };
    
    class SI32Desc : public IntegerDescT<int32_t>
    {
      friend class Manager;
      
    protected:
    
      SI32Desc( std::string const &name, RC::ConstHandle<SI32Impl> const &integerImpl )
        : IntegerDescT<int32_t>( name, integerImpl )
      {
      }
    };
    
    class SizeDesc : public IntegerDescT<size_t>
    {
      friend class Manager;
      
    protected:
    
      SizeDesc( std::string const &name, RC::ConstHandle<SizeImpl> const &sizeImpl )
        : IntegerDescT<size_t>( name, sizeImpl )
      {
      }
    };
  };
};

#endif //_FABRIC_RT_INTEGER_DESC_H
