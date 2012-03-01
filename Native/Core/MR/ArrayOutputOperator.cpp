/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/ArrayOutputOperator.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace MR
  {
#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK() \
          do \
          { \
            if ( numParams == 3 ) \
              m_call3 = &ArrayOutputOperator::call3; \
            else if ( numParams == 2 ) \
              m_call2 = &ArrayOutputOperator::call2; \
            else \
              m_call1 = &ArrayOutputOperator::call1; \
          } while ( false )

#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( sharedTypeName ) \
          do \
          { \
            if ( numParams == 4 ) \
              m_call4 = &ArrayOutputOperator::call4##sharedTypeName; \
            else FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK(); \
          } while ( false )

#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_SD( sharedDesc ) \
        do \
        { \
          if ( sharedDesc ) \
          { \
            RT::ImplType sharedType = sharedDesc->getType(); \
            if ( RT::isBoolean(sharedType) ) \
              FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( Boolean ); \
            else if ( RT::isInteger(sharedType) ) \
            { \
              size_t size = sharedDesc->getAllocSize(); \
              bool isSigned = RC::ConstHandle<RT::IntegerDesc>::StaticCast( sharedDesc )->isSigned(); \
              switch ( size ) \
              { \
                case 1: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( SInt8 ); \
                  else \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( UInt8 ); \
                  break; \
                case 2: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( SInt16 ); \
                  else \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( UInt16 ); \
                  break; \
                case 4: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( SInt32 ); \
                  else \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( UInt32 ); \
                  break; \
                case 8: \
                  if ( isSigned ) \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( SInt64 ); \
                  else \
                    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( UInt64 ); \
                  break; \
                default: \
                  FABRIC_ASSERT( false && "Unsupported integer size" ); \
              } \
            } \
            else if ( RT::isFloat(sharedType) ) \
            { \
              if ( sharedDesc->getAllocSize() == 8 ) \
                FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( Float64 ); \
              else \
                FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( Float32 ); \
            } \
            else FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_STN( Default ); \
          } \
          else FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK(); \
        } while ( false )

    ArrayOutputOperator::ArrayOutputOperator()
      : m_functionPtr( 0 )
      , m_call1( 0 )
      , m_call2( 0 )
      , m_call3( 0 )
      , m_call4( 0 )
    {
    }
           
    void ArrayOutputOperator::init(
      void (*functionPtr)(...),
      size_t numParams,
      RC::ConstHandle<RT::Desc> const &valueDesc,
      RC::ConstHandle<RT::Desc> const &sharedDesc
      )
    {
      m_functionPtr = functionPtr;
      m_valueDesc = valueDesc;
      m_sharedDesc = sharedDesc;

      if ( m_functionPtr )
        FABRIC_MR_ARRAY_OUTPUT_OPERATOR_PICK_SD( sharedDesc );
    }

    Exception ArrayOutputOperator::GetPrototypeException()
    {
      static const Exception exception("operator must have prototype: io ValueType valueData[, in Size index[,in Size count[, in SharedType sharedData]]]]");
      return exception;
    }
        
    RC::ConstHandle<RT::Desc> ArrayOutputOperator::getValueDesc() const
    {
      if ( !m_valueDesc )
        throw Exception( "error in operator" );
      return m_valueDesc;
    }
    
    RC::ConstHandle<RT::Desc> ArrayOutputOperator::getSharedDesc() const
    {
      return m_sharedDesc;
    }
    
#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( sharedTypeName, sharedTypeCType ) \
    void ArrayOutputOperator::call4##sharedTypeName( void *valueData, size_t index, size_t count, void const *sharedData ) \
    { \
      ((void (*)( void *, size_t, size_t, sharedTypeCType ))m_functionPtr)( valueData, index, count, *(sharedTypeCType const *)sharedData ); \
    } \
    
#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_DEFAULT() \
    void ArrayOutputOperator::call4Default( void *valueData, size_t index, size_t count, void const *sharedData ) \
    { \
      ((void (*)( void *, size_t, size_t, void const * ))m_functionPtr)( valueData, index, count, sharedData ); \
    } \

#define FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL() \
    void ArrayOutputOperator::call1( void *valueData ) \
    { \
      ((void (*)( void * ))m_functionPtr)( valueData ); \
    } \
    void ArrayOutputOperator::call2( void *valueData, size_t index ) \
    { \
      ((void (*)( void *, size_t ))m_functionPtr)( valueData, index ); \
    } \
    void ArrayOutputOperator::call3( void *valueData, size_t index, size_t count ) \
    { \
      ((void (*)( void *, size_t, size_t ))m_functionPtr)( valueData, index, count ); \
    } \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( Boolean, bool ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( UInt8, uint8_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( SInt8, int8_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( UInt16, uint16_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( SInt16, int16_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( UInt32, uint32_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( SInt32, int32_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( UInt64, uint64_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( SInt64, int64_t ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( Float32, float ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_ST( Float64, double ) \
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL_DEFAULT() \
    
    FABRIC_MR_ARRAY_OUTPUT_OPERATOR_CALL_IMPL()
    
    void ArrayOutputOperator::call(
      void *valueData
      ) const
    {
      FABRIC_ASSERT( m_call1 );
      (const_cast<ArrayOutputOperator *>(this)->*m_call1)( valueData );
    }
    
    void ArrayOutputOperator::call(
      void *valueData,
      size_t index
      ) const
    {
      FABRIC_ASSERT( m_call2 );
      (const_cast<ArrayOutputOperator *>(this)->*m_call2)( valueData, index );
    }
    
    void ArrayOutputOperator::call(
      void *valueData,
      size_t index,
      size_t count
      ) const
    {
      FABRIC_ASSERT( m_call3 );
      (const_cast<ArrayOutputOperator *>(this)->*m_call3)( valueData, index, count );
    }
    
    void ArrayOutputOperator::call(
      void *valueData,
      size_t index,
      size_t count,
      void const *sharedData
      ) const
    {
      FABRIC_ASSERT( m_call4 );
      (const_cast<ArrayOutputOperator *>(this)->*m_call4)( valueData, index, count, sharedData );
    }
  }
}
