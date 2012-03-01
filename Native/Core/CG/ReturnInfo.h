/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_RETURN_INFO_H
#define _FABRIC_CG_RETURN_INFO_H

#include <Fabric/Core/CG/ExprType.h>

namespace Fabric
{
  namespace CG
  {
    class ReturnInfo
    {
    public:
    
      ReturnInfo( ExprType const &exprType, llvm::Value *returnLValue = NULL )
        : m_exprType( exprType )
        , m_returnLValue( returnLValue )
      {
      }

      ReturnInfo( ReturnInfo const &that )
        : m_exprType( that.m_exprType )
        , m_returnLValue( that.m_returnLValue )
      {
      }

      ReturnInfo &operator =( ReturnInfo const &that )
      {
        m_exprType = that.m_exprType;
        m_returnLValue = that.m_returnLValue;
        return *this;
      }

      operator bool() const
      {
        return m_exprType.isValid();
      }
      
      bool operator !() const
      {
        return !m_exprType.isValid();
      }

      ExprType const &getExprType() const
      {
        return m_exprType;
      }

      RC::ConstHandle<Adapter> getAdapter() const
      {
        return m_exprType.getAdapter();
      }

      CG::Usage getUsage() const
      {
        return m_exprType.getUsage();
      }

      bool usesReturnLValue() const
      {
        if ( m_exprType.isValid() )
          return m_exprType.getAdapter()->usesReturnLValue();
        else return false;
      }

      llvm::Value *getReturnLValue( ) const
      {
        return m_returnLValue;
      }

    private:

      ExprType m_exprType;
      llvm::Value *m_returnLValue;
    };
  }
}

#endif //_FABRIC_CG_RETURN_INFO_H
