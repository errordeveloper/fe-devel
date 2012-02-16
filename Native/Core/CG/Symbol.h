/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_SYMBOL_H
#define _FABRIC_CG_SYMBOL_H

#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/ExprValue.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/FunctionParam.h>
#include <Fabric/Core/CG/ReturnInfo.h>
#include <Fabric/Core/Util/UnorderedMap.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace CG
  {
    class Symbol : public RC::Object
    {
    public:
    
      virtual bool isValue() const { return false; }
      virtual bool isPencil() const { return false; }
    };
    
    class ValueSymbol : public Symbol
    {
    public:
    
      virtual bool isValue() const { return true; }
      virtual bool isVariable() const { return false; }
      virtual bool isParameter() const { return false; }
      virtual bool isConstant() const { return false; }
      
      CG::ExprValue const &getExprValue() const
      {
        return m_exprValue;
      }
      
      RC::ConstHandle<CG::Adapter> getAdapter() const
      {
        return m_exprValue.getAdapter();
      }
      
    protected:
    
      ValueSymbol( CG::ExprValue const &exprValue )
        : m_exprValue( exprValue )
      {
      }
      
    private:
    
      CG::ExprValue m_exprValue;
    };
    
    class VariableSymbol : public ValueSymbol
    {
    public:
    
      static RC::ConstHandle<VariableSymbol> Create( CG::ExprValue const &exprValue )
      {
        return new VariableSymbol( exprValue );
      }
      
      virtual bool isVariable() const { return true; }
      
    protected:
    
      VariableSymbol( CG::ExprValue const &exprValue )
        : ValueSymbol( exprValue )
      {
      }
    };
    
    class ParameterSymbol : public ValueSymbol
    {
    public:
    
      static RC::ConstHandle<ParameterSymbol> Create( CG::ExprValue const &exprValue )
      {
        return new ParameterSymbol( exprValue );
      }
      
      virtual bool isParameter() const { return true; }
      
    protected:
    
      ParameterSymbol( CG::ExprValue const &exprValue )
        : ValueSymbol( exprValue )
      {
      }
    };
    
    class ConstantSymbol : public ValueSymbol
    {
    public:
    
      static RC::ConstHandle<ConstantSymbol> Create( CG::ExprValue const &exprValue )
      {
        return new ConstantSymbol( exprValue );
      }
      
      virtual bool isConstant() const { return true; }
      
    protected:
    
      ConstantSymbol( CG::ExprValue const &exprValue )
        : ValueSymbol( exprValue )
      {
      }
    };
  }
}

#endif //_FABRIC_CG_SYMBOL_H
