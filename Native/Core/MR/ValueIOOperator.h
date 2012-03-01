/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_VALUE_IO_OPERATOR_H
#define _FABRIC_MR_VALUE_IO_OPERATOR_H

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
#define FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, sharedTypeName ) \
      void call3##inputTypeName##sharedTypeName( void const *inputData, void *outputData, void const *sharedData ); \
      
#define FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( inputTypeName ) \
      void call2##inputTypeName( void const *inputData, void *outputData ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Boolean ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt8 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt8 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt16 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt16 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt32 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt32 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt64 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt64 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Float32 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Float64 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Default ); \
      
#define FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL() \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( Boolean ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( UInt8 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( SInt8 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( UInt16 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( SInt16 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( UInt32 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( SInt32 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( UInt64 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( SInt64 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( Float32 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( Float64 ); \
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL_ITN( Default ); \

    class ValueIOOperator : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      RC::ConstHandle<RT::Desc> getInputDesc() const;
      RC::ConstHandle<RT::Desc> getOutputDesc() const;
      RC::ConstHandle<RT::Desc> getSharedDesc() const;
      
      bool takesSharedValue() const
      {
        return !!m_call3;
      }
      
      void call(
        void const *inputData,
        void *outputData
        ) const;
      void call(
        void const *inputData,
        void *outputData,
        void const *sharedData
        ) const;
      
    protected:
    
      ValueIOOperator();
      
      void init(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &inputDesc,
        RC::ConstHandle<RT::Desc> const &outputDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
      
      static Exception GetPrototypeException();
    
    private:
    
      FABRIC_MR_VALUE_IO_OPERATOR_CALL_DECL()
    
      void (*m_functionPtr)(...);
      RC::ConstHandle<RT::Desc> m_inputDesc;
      RC::ConstHandle<RT::Desc> m_outputDesc;
      RC::ConstHandle<RT::Desc> m_sharedDesc;
      
      void (ValueIOOperator::*m_call2)( void const *inputData, void *outputData );
      void (ValueIOOperator::*m_call3)( void const *inputData, void *outputData, void const *sharedData );
    };
  }
}

#endif //_FABRIC_MR_VALUE_IO_OPERATOR_H
