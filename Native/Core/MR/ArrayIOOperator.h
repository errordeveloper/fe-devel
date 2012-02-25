/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_ARRAY_IO_OPERATOR_H
#define _FABRIC_MR_ARRAY_IO_OPERATOR_H

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
#define FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, sharedTypeName ) \
      void call5##inputTypeName##sharedTypeName( void const *inputData, void *outputData, size_t index, size_t count, void const *sharedData ); \
      
#define FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( inputTypeName ) \
      void call2##inputTypeName( void const *inputData, void *outputData ); \
      void call3##inputTypeName( void const *inputData, void *outputData, size_t index ); \
      void call4##inputTypeName( void const *inputData, void *outputData, size_t index, size_t count ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Boolean ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt8 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt8 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt16 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt16 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt32 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt32 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, UInt64 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, SInt64 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Float32 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Float64 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN_STN( inputTypeName, Default ); \
      
#define FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL() \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( Boolean ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( UInt8 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( SInt8 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( UInt16 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( SInt16 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( UInt32 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( SInt32 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( UInt64 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( SInt64 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( Float32 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( Float64 ); \
      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL_ITN( Default ); \

    class ArrayIOOperator : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      RC::ConstHandle<RT::Desc> getInputDesc() const;
      RC::ConstHandle<RT::Desc> getOutputDesc() const;
      RC::ConstHandle<RT::Desc> getSharedDesc() const;
      
      bool takesIndex() const
      {
        return m_call3 != 0 || m_call4 != 0 || m_call5 != 0;
      }
      bool takesCount() const
      {
        return m_call4 != 0 || m_call5 != 0;
      }
      bool takesSharedValue() const
      {
        return m_call5 != 0;
      }
      
      void call(
        void const *inputData,
        void *outputData
        ) const;
      void call(
        void const *inputData,
        void *outputData,
        size_t index
        ) const;
      void call(
        void const *inputData,
        void *outputData,
        size_t index,
        size_t count
        ) const;
      void call(
        void const *inputData,
        void *outputData,
        size_t index,
        size_t count,
        void const *sharedData
        ) const;
      
      static Exception GetPrototypeException();
      
    protected:
    
      ArrayIOOperator();
      
      void init(
        void (*functionPtr)(...),
        size_t numParams,
        RC::ConstHandle<RT::Desc> const &inputDesc,
        RC::ConstHandle<RT::Desc> const &outputDesc,
        RC::ConstHandle<RT::Desc> const &sharedDesc
        );
    
    private:

      FABRIC_KLC_ARRAY_IO_OPERATOR_CALL_DECL()
    
      void (*m_functionPtr)( ... );
      RC::ConstHandle<RT::Desc> m_inputDesc;
      RC::ConstHandle<RT::Desc> m_outputDesc;
      RC::ConstHandle<RT::Desc> m_sharedDesc;
      
      void (ArrayIOOperator::*m_call2)( void const *inputData, void *outputData );
      void (ArrayIOOperator::*m_call3)( void const *inputData, void *outputData, size_t index );
      void (ArrayIOOperator::*m_call4)( void const *inputData, void *outputData, size_t index, size_t count );
      void (ArrayIOOperator::*m_call5)( void const *inputData, void *outputData, size_t index, size_t count, void const *sharedData );
    };
  }
}

#endif //_FABRIC_MR_ARRAY_IO_OPERATOR_H
