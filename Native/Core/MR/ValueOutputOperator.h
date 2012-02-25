/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_OUTPUT_OPERATOR_H
#define _FABRIC_MR_VALUE_OUTPUT_OPERATOR_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    class Desc;
  }
  
  namespace MR
  {
#define FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( sharedTypeName ) \
      void call2##sharedTypeName( void *data, void const *sharedData ); \
      
#define FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL() \
      void call1( void *data ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( Boolean ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( UInt8 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( SInt8 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( UInt16 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( SInt16 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( UInt32 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( SInt32 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( UInt64 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( SInt64 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( Float32 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( Float64 ); \
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL_STN( Default ); \

    class ValueOutputOperator : public RC::Object
    {
    public:
      REPORT_RC_LEAKS

      RC::ConstHandle<RT::Desc> getValueDesc() const;
      RC::ConstHandle<RT::Desc> getSharedDesc() const;
      
      bool takesSharedValue() const
      {
        return !!m_call2;
      }
      
      void call(
        void *valueData
        ) const;
      void call(
        void *valueData,
        void const *sharedData
        ) const;
      
      static Exception GetPrototypeException();
      
    protected:
    
      ValueOutputOperator();
      
      void init(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
    
    private:
    
      FABRIC_MR_VALUE_OUTPUT_OPERATOR_CALL_DECL()
    
      void (*m_functionPtr)(...);
      RC::ConstHandle<RT::Desc> m_valueDesc;
      RC::ConstHandle<RT::Desc> m_sharedDesc;
      
      void (ValueOutputOperator::*m_call1)( void *valueData );
      void (ValueOutputOperator::*m_call2)( void *valueData, void const *sharedData );
    };
  }
}

#endif //_FABRIC_MR_VALUE_OUTPUT_OPERATOR_H
