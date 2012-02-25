/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_OUTPUT_OPERATOR_H
#define _FABRIC_MR_ARRAY_OUTPUT_OPERATOR_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    class Desc;
  };
  
  namespace MR
  {
#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( sharedTypeName ) \
      void call4##sharedTypeName( void *valueData, size_t index, size_t count, void const *sharedData ); \
      
#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL() \
      void call1( void *valueData ); \
      void call2( void *valueData, size_t index ); \
      void call3( void *valueData, size_t index, size_t count ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( Boolean ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( UInt8 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( SInt8 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( UInt16 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( SInt16 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( UInt32 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( SInt32 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( UInt64 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( SInt64 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( Float32 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( Float64 ); \
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL_STN( Default ); \

    class ArrayOutputOperator : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      RC::ConstHandle<RT::Desc> getValueDesc() const;
      RC::ConstHandle<RT::Desc> getSharedDesc() const;
      
      bool takesIndex() const
      {
        return m_call2 != 0 || m_call3 != 0 || m_call4 != 0;
      }
      bool takesCount() const
      {
        return m_call3 != 0 || m_call4 != 0;
      }
      bool takesSharedValue() const
      {
        return m_call4 != 0;
      }
      
      void call(
        void *valueData
        ) const;
      void call(
        void *valueData,
        size_t index
        ) const;
      void call(
        void *valueData,
        size_t index,
        size_t count
        ) const;
      void call(
        void *valueData,
        size_t index,
        size_t count,
        void const *sharedData
        ) const;
      
      static Exception GetPrototypeException();
      
    protected:
    
      ArrayOutputOperator();
      
      void init(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &valueDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
    
    private:
    
      FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_DECL()
    
      void (*m_functionPtr)( ... );
      RC::ConstHandle<RT::Desc> m_valueDesc;
      RC::ConstHandle<RT::Desc> m_sharedDesc;
      
      void (ArrayOutputOperator::*m_call1)( void *valueData );
      void (ArrayOutputOperator::*m_call2)( void *valueData, size_t index );
      void (ArrayOutputOperator::*m_call3)( void *valueData, size_t index, size_t count );
      void (ArrayOutputOperator::*m_call4)( void *valueData, size_t index, size_t count, void const *sharedData );
    };
  }
}

#endif //_FABRIC_KL_ARRAY_TRANSFORM_OPERATOR_H
