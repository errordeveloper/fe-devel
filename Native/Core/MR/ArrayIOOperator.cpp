/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayIOOperator.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
#define FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN( inputTypeName ) \
          do \
          { \
            if ( numParams == 4 ) \
              m_call4 = &ArrayIOOperator::call4##inputTypeName; \
            else if ( numParams == 3 ) \
              m_call3 = &ArrayIOOperator::call3##inputTypeName; \
            else \
              m_call2 = &ArrayIOOperator::call2##inputTypeName; \
          } while ( false )

#define FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, sharedTypeName ) \
          do \
          { \
            if ( numParams == 5 ) \
              m_call5 = &ArrayIOOperator::call5##inputTypeName##sharedTypeName; \
            else FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN( inputTypeName ); \
          } while ( false )

#define FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( inputTypeName, sharedDesc ) \
        do \
        { \
          if ( sharedDesc ) \
          { \
            RT::ImplType sharedType = sharedDesc->getType(); \
            if ( RT::isBoolean(sharedType) ) \
              FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, Boolean ); \
            else if ( RT::isInteger(sharedType) ) \
            { \
              size_t size = sharedDesc->getAllocSize(); \
              bool isSigned = RC::ConstHandle<RT::IntegerDesc>::StaticCast( sharedDesc )->isSigned(); \
              switch ( size ) \
              { \
                case 1: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, SInt8 ); \
                  else \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, UInt8 ); \
                  break; \
                case 2: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, SInt16 ); \
                  else \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, UInt16 ); \
                  break; \
                case 4: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, SInt32 ); \
                  else \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, UInt32 ); \
                  break; \
                case 8: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, SInt64 ); \
                  else \
                    FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, UInt64 ); \
                  break; \
                default: \
                  FABRIC_ASSERT( false && "Unsupported integer size" ); \
              } \
            } \
            else if ( RT::isFloat(sharedType) ) \
            { \
              if ( sharedDesc->getAllocSize() == 8 ) \
                FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, Float64 ); \
              else \
                FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, Float32 ); \
            } \
            else FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_STN( inputTypeName, Default ); \
          } \
          else FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN( inputTypeName ); \
        } while ( false )

#define FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ID_SD( inputDesc, sharedDesc ) \
        do \
        { \
          RT::ImplType inputType = inputDesc->getType(); \
          if ( RT::isBoolean(inputType) ) \
            FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( Boolean, sharedDesc ); \
          else if ( RT::isInteger(inputType) ) \
          { \
            size_t size = inputDesc->getAllocSize(); \
            bool isSigned = RC::ConstHandle<RT::IntegerDesc>::StaticCast( inputDesc )->isSigned(); \
            switch ( size ) \
            { \
              case 1: \
                if ( isSigned ) \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( SInt8, sharedDesc ); \
                else \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( UInt8, sharedDesc ); \
                break; \
              case 2: \
                if ( isSigned ) \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( SInt16, sharedDesc ); \
                else \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( UInt16, sharedDesc ); \
                break; \
              case 4: \
                if ( isSigned ) \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( SInt32, sharedDesc ); \
                else \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( UInt32, sharedDesc ); \
                break; \
              case 8: \
                if ( isSigned ) \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( SInt64, sharedDesc ); \
                else \
                  FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( UInt64, sharedDesc ); \
                break; \
              default: \
                FABRIC_ASSERT( false && "Unsupported integer size" ); \
            } \
          } \
          else if ( RT::isFloat(inputType) ) \
          { \
            if ( inputDesc->getAllocSize() == 8 ) \
              FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( Float64, sharedDesc ); \
            else \
              FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( Float32, sharedDesc ); \
          } \
          else FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ITN_SD( Default, sharedDesc ); \
        } while ( false )

    ArrayIOOperator::ArrayIOOperator()
      : m_functionPtr( 0 )
      , m_call2( 0 )
      , m_call3( 0 )
      , m_call4( 0 )
      , m_call5( 0 )
    {
    }
      
    void ArrayIOOperator::init(
      void (*functionPtr)(...),
      size_t numParams,
      RC::ConstHandle<RT::Desc> const &inputDesc,
      RC::ConstHandle<RT::Desc> const &outputDesc,
      RC::ConstHandle<RT::Desc> const &sharedDesc
      )
    {
      m_functionPtr = functionPtr;
      m_inputDesc = inputDesc;
      m_outputDesc = outputDesc;
      m_sharedDesc = sharedDesc;

      if ( m_functionPtr )
        FABRIC_MR_ARRAY_IO_OPERATOR_PICK_ID_SD( inputDesc, sharedDesc );
    }

    Exception ArrayIOOperator::GetPrototypeException()
    {
      static const Exception exception("operator must have prototype: in InputType inputData, io OutputType outputData[, in Size index[,in Size count[, in SharedType sharedData]]]]");
      return exception;
    }
        
    RC::ConstHandle<RT::Desc> ArrayIOOperator::getInputDesc() const
    {
      return m_inputDesc;
    }
    
    RC::ConstHandle<RT::Desc> ArrayIOOperator::getOutputDesc() const
    {
      return m_outputDesc;
    }
    
    RC::ConstHandle<RT::Desc> ArrayIOOperator::getSharedDesc() const
    {
      return m_sharedDesc;
    }
    
#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, sharedTypeName, sharedTypeCType ) \
    void ArrayIOOperator::call5##inputTypeName##sharedTypeName( void const *inputData, void *outputData, size_t index, size_t count, void const *sharedData ) \
    { \
      ((void (*)( inputTypeCType, void *, size_t, size_t, sharedTypeCType ))m_functionPtr)( *(inputTypeCType const *)inputData, outputData, index, count, *(sharedTypeCType const *)sharedData ); \
    } \
    
#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_DEFAULT( inputTypeName, inputTypeCType ) \
    void ArrayIOOperator::call5##inputTypeName##Default( void const *inputData, void *outputData, size_t index, size_t count, void const *sharedData ) \
    { \
      ((void (*)( inputTypeCType, void *, size_t, size_t, void const * ))m_functionPtr)( *(inputTypeCType const *)inputData, outputData, index, count, sharedData ); \
    } \

#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( inputTypeName, inputTypeCType ) \
    void ArrayIOOperator::call2##inputTypeName( void const *inputData, void *outputData ) \
    { \
      ((void (*)( inputTypeCType, void * ))m_functionPtr)( *(inputTypeCType const *)inputData, outputData ); \
    } \
    void ArrayIOOperator::call3##inputTypeName( void const *inputData, void *outputData, size_t index ) \
    { \
      ((void (*)( inputTypeCType, void *, size_t ))m_functionPtr)( *(inputTypeCType const *)inputData, outputData, index ); \
    } \
    void ArrayIOOperator::call4##inputTypeName( void const *inputData, void *outputData, size_t index, size_t count ) \
    { \
      ((void (*)( inputTypeCType, void *, size_t, size_t ))m_functionPtr)( *(inputTypeCType const *)inputData, outputData, index, count ); \
    } \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, Boolean, bool ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, UInt8, uint8_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, SInt8, int8_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, UInt16, uint16_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, SInt16, int16_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, UInt32, uint32_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, SInt32, int32_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, UInt64, uint64_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, SInt64, int64_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, Float32, float ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_ST( inputTypeName, inputTypeCType, Float64, double ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT_DEFAULT( inputTypeName, inputTypeCType ) \
    
#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( sharedTypeName, sharedTypeCType ) \
    void ArrayIOOperator::call5Default##sharedTypeName( void const *inputData, void *outputData, size_t index, size_t count, void const *sharedData ) \
    { \
      ((void (*)( void const *, void *, size_t, size_t, sharedTypeCType ))m_functionPtr)( inputData, outputData, index, count, *(sharedTypeCType const *)sharedData ); \
    } \

#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_DEFAULT() \
    void ArrayIOOperator::call5DefaultDefault( void const *inputData, void *outputData, size_t index, size_t count, void const *sharedData ) \
    { \
      m_functionPtr( inputData, outputData, index, count, sharedData ); \
    } \

#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT() \
    void ArrayIOOperator::call2Default( void const *inputData, void *outputData ) \
    { \
      m_functionPtr( inputData, outputData ); \
    } \
    void ArrayIOOperator::call3Default( void const *inputData, void *outputData, size_t index ) \
    { \
      m_functionPtr( inputData, outputData, index ); \
    } \
    void ArrayIOOperator::call4Default( void const *inputData, void *outputData, size_t index, size_t count ) \
    { \
      m_functionPtr( inputData, outputData, index, count ); \
    } \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( Boolean, bool ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( UInt8, uint8_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( SInt8, int8_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( UInt16, uint16_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( SInt16, int16_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( UInt32, uint32_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( SInt32, int32_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( UInt64, uint64_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( SInt64, int64_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( Float32, float ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_ST( Float64, double ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT_DEFAULT() \

#define FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL() \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( Boolean, bool ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( UInt8, uint8_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( SInt8, int8_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( UInt16, uint16_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( SInt16, int16_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( UInt32, uint32_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( SInt32, int32_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( UInt64, uint64_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( SInt64, int64_t ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( Float32, float ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_IT( Float64, double ) \
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL_DEFAULT() \
    
    FABRIC_MR_ARRAY_IO_OPERATOR_CALL_IMPL()
    
    void ArrayIOOperator::call(
      void const *inputData,
      void *outputData
      ) const
    {
      FABRIC_ASSERT( m_call2 );
      (const_cast<ArrayIOOperator *>(this)->*m_call2)( inputData, outputData );
    }
    
    void ArrayIOOperator::call(
      void const *inputData,
      void *outputData,
      size_t index
      ) const
    {
      FABRIC_ASSERT( m_call3 );
      (const_cast<ArrayIOOperator *>(this)->*m_call3)( inputData, outputData, index );
    }
    
    void ArrayIOOperator::call(
      void const *inputData,
      void *outputData,
      size_t index,
      size_t count
      ) const
    {
      FABRIC_ASSERT( m_call4 );
      (const_cast<ArrayIOOperator *>(this)->*m_call4)( inputData, outputData, index, count );
    }
    
    void ArrayIOOperator::call(
      void const *inputData,
      void *outputData,
      size_t index,
      size_t count,
      void const *sharedData
      ) const
    {
      FABRIC_ASSERT( m_call5 );
      (const_cast<ArrayIOOperator *>(this)->*m_call5)( inputData, outputData, index, count, sharedData );
    }
  }
}
