/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_OP_TYPES_H
#define _FABRIC_CG_OP_TYPES_H

#include <Fabric/Base/Util/Assert.h>

#include <string>
#include <vector>

namespace Fabric
{
  namespace CG
  {
    enum UniOpType
    {
      UNI_OP_POS,
      UNI_OP_NEG,
      UNI_OP_BIT_NOT,
      UNI_OP_PRE_INC,
      UNI_OP_PRE_DEC,
      UNI_OP_POST_INC,
      UNI_OP_POST_DEC,
      NUM_UNI_OPS
    };

    inline std::string const &uniOpCodeName( UniOpType uniOpType )
    {
      static std::vector<std::string> uniOpCodeNames;
      if ( uniOpCodeNames.empty() )
      {
        uniOpCodeNames.resize( NUM_UNI_OPS );
        uniOpCodeNames[UNI_OP_POS] = "POS";
        uniOpCodeNames[UNI_OP_NEG] = "NEG";
        uniOpCodeNames[UNI_OP_BIT_NOT] = "BIT_NOT";
        uniOpCodeNames[UNI_OP_PRE_INC] = "PRE_INC";
        uniOpCodeNames[UNI_OP_PRE_DEC] = "PRE_DEC";
        uniOpCodeNames[UNI_OP_POST_INC] = "POST_INC";
        uniOpCodeNames[UNI_OP_POST_DEC] = "POST_DEC";
      }
      std::string const &result = uniOpCodeNames[uniOpType];
      FABRIC_ASSERT( result.length() > 0 );
      return result;
    }

    inline std::string const &uniOpUserName( UniOpType uniOpType )
    {
      static std::vector<std::string> uniOpUserNames;
      if ( uniOpUserNames.empty() )
      {
        uniOpUserNames.resize( NUM_UNI_OPS );
        uniOpUserNames[UNI_OP_POS] = "+";
        uniOpUserNames[UNI_OP_NEG] = "-";
        uniOpUserNames[UNI_OP_BIT_NOT] = "~";
        uniOpUserNames[UNI_OP_PRE_INC] = "prefix ++";
        uniOpUserNames[UNI_OP_PRE_DEC] = "prefix --";
        uniOpUserNames[UNI_OP_POST_INC] = "postfix ++";
        uniOpUserNames[UNI_OP_POST_DEC] = "postfix --";
      }
      std::string const &result = uniOpUserNames[uniOpType];
      FABRIC_ASSERT( result.length() > 0 );
      return result;
    }
    
    enum BinOpType
    {
      BIN_OP_ADD,
      BIN_OP_SUB,
      BIN_OP_MUL,
      BIN_OP_DIV,
      BIN_OP_MOD,
      BIN_OP_EQ,
      BIN_OP_NE,
      BIN_OP_LT,
      BIN_OP_LE,
      BIN_OP_GT,
      BIN_OP_GE,
      BIN_OP_BIT_OR,
      BIN_OP_BIT_AND,
      BIN_OP_BIT_XOR,
      BIN_OP_SHL,
      BIN_OP_SHR,
      NUM_BIN_OPS
    };

    inline std::string const &binOpCodeName( BinOpType binOpType )
    {
      static std::vector<std::string> binOpCodeNames;
      if ( binOpCodeNames.empty() )
      {
        binOpCodeNames.resize( NUM_BIN_OPS );
        binOpCodeNames[BIN_OP_ADD] = "add";
        binOpCodeNames[BIN_OP_SUB] = "sub";
        binOpCodeNames[BIN_OP_MUL] = "mul";
        binOpCodeNames[BIN_OP_DIV] = "div";
        binOpCodeNames[BIN_OP_MOD] = "mod";
        binOpCodeNames[BIN_OP_EQ] = "eq";
        binOpCodeNames[BIN_OP_NE] = "ne";
        binOpCodeNames[BIN_OP_LT] = "lt";
        binOpCodeNames[BIN_OP_LE] = "le";
        binOpCodeNames[BIN_OP_GT] = "gt";
        binOpCodeNames[BIN_OP_GE] = "ge";
        binOpCodeNames[BIN_OP_BIT_OR] = "bit_or";
        binOpCodeNames[BIN_OP_BIT_AND] = "bit_and";
        binOpCodeNames[BIN_OP_BIT_XOR] = "bit_xor";
        binOpCodeNames[BIN_OP_SHL] = "shl";
        binOpCodeNames[BIN_OP_SHR] = "shr";
      }
      std::string const &result = binOpCodeNames[binOpType];
      FABRIC_ASSERT( result.length() > 0 );
      return result;
    }

    inline std::string const &binOpUserName( BinOpType binOpType )
    {
      static std::vector<std::string> binOpUserNames;
      if ( binOpUserNames.empty() )
      {
        binOpUserNames.resize( NUM_BIN_OPS );
        binOpUserNames[BIN_OP_ADD] = "+";
        binOpUserNames[BIN_OP_SUB] = "-";
        binOpUserNames[BIN_OP_MUL] = "*";
        binOpUserNames[BIN_OP_DIV] = "/";
        binOpUserNames[BIN_OP_MOD] = "%";
        binOpUserNames[BIN_OP_EQ] = "==";
        binOpUserNames[BIN_OP_NE] = "!=";
        binOpUserNames[BIN_OP_LT] = "<";
        binOpUserNames[BIN_OP_LE] = "<=";
        binOpUserNames[BIN_OP_GT] = ">";
        binOpUserNames[BIN_OP_GE] = ">=";
        binOpUserNames[BIN_OP_BIT_OR] = "|";
        binOpUserNames[BIN_OP_BIT_AND] = "&";
        binOpUserNames[BIN_OP_BIT_XOR] = "^";
        binOpUserNames[BIN_OP_SHL] = "<<";
        binOpUserNames[BIN_OP_SHR] = ">>";
      }
      std::string const &result = binOpUserNames[binOpType];
      FABRIC_ASSERT( result.length() > 0 );
      return result;
    }
    
#define FABRIC_CG_OP_TYPES_HANDLE_DESC( value ) \
  case value: \
  { \
    static std::string desc = #value; \
    return desc; \
  }
#define FABRIC_CG_OP_TYPES_HANDLE_DEFAULT() \
  default: \
  { \
    FABRIC_ASSERT( false && "Unhandled value" ); \
    static std::string desc = "<INTERNAL ERROR>"; \
    return desc; \
  }
    
    enum TernaryOpType
    {
      TERNARY_OP_COND
    };
    
    inline char const *ternaryOpTypeDesc( TernaryOpType ternaryOpType )
    {
      switch( ternaryOpType )
      {
        case TERNARY_OP_COND:
          return "TERNARY_OP_COND";
      }
      return "<unknown>";
    }
    
    enum AssignOpType
    {
      ASSIGN_OP,
      ASSIGN_OP_ADD,
      ASSIGN_OP_SUB,
      ASSIGN_OP_MUL,
      ASSIGN_OP_DIV,
      ASSIGN_OP_MOD,
      ASSIGN_OP_BIT_OR,
      ASSIGN_OP_BIT_AND,
      ASSIGN_OP_BIT_XOR,
      ASSIGN_OP_SHL,
      ASSIGN_OP_SHR,
      NUM_ASSIGN_OPS
    };
    
    inline std::string const &assignOpCodeName( AssignOpType assignOpType )
    {
      static std::vector<std::string> assignOpCodeNames;
      if ( assignOpCodeNames.empty() )
      {
        assignOpCodeNames.resize( NUM_ASSIGN_OPS );
        assignOpCodeNames[ASSIGN_OP] = "reg";
        assignOpCodeNames[ASSIGN_OP_ADD] = "add";
        assignOpCodeNames[ASSIGN_OP_SUB] = "sub";
        assignOpCodeNames[ASSIGN_OP_MUL] = "mul";
        assignOpCodeNames[ASSIGN_OP_DIV] = "div";
        assignOpCodeNames[ASSIGN_OP_MOD] = "mod";
        assignOpCodeNames[ASSIGN_OP_BIT_OR] = "bit_or";
        assignOpCodeNames[ASSIGN_OP_BIT_AND] = "bit_and";
        assignOpCodeNames[ASSIGN_OP_BIT_XOR] = "bit_xor";
        assignOpCodeNames[ASSIGN_OP_SHL] = "shl";
        assignOpCodeNames[ASSIGN_OP_SHR] = "shr";
      }
      std::string const &result = assignOpCodeNames[assignOpType];
      FABRIC_ASSERT( result.length() > 0 );
      return result;
    }
    
    inline std::string const &assignOpUserName( AssignOpType assignOpType )
    {
      static std::vector<std::string> assignOpCodeNames;
      if ( assignOpCodeNames.empty() )
      {
        assignOpCodeNames.resize( NUM_ASSIGN_OPS );
        assignOpCodeNames[ASSIGN_OP] = "=";
        assignOpCodeNames[ASSIGN_OP_ADD] = "+=";
        assignOpCodeNames[ASSIGN_OP_SUB] = "-=";
        assignOpCodeNames[ASSIGN_OP_MUL] = "*=";
        assignOpCodeNames[ASSIGN_OP_DIV] = "/=";
        assignOpCodeNames[ASSIGN_OP_MOD] = "%=";
        assignOpCodeNames[ASSIGN_OP_BIT_OR] = "|=";
        assignOpCodeNames[ASSIGN_OP_BIT_AND] = "&=";
        assignOpCodeNames[ASSIGN_OP_BIT_XOR] = "^=";
        assignOpCodeNames[ASSIGN_OP_SHL] = "<<=";
        assignOpCodeNames[ASSIGN_OP_SHR] = ">>=";
      }
      std::string const &result = assignOpCodeNames[assignOpType];
      FABRIC_ASSERT( result.length() > 0 );
      return result;
    }
    
    inline BinOpType binOpForAssignOp( AssignOpType assignOpType )
    {
      switch ( assignOpType )
      {
        case ASSIGN_OP_ADD: return BIN_OP_ADD;
        case ASSIGN_OP_SUB: return BIN_OP_SUB;
        case ASSIGN_OP_MUL: return BIN_OP_MUL;
        case ASSIGN_OP_DIV: return BIN_OP_DIV;
        case ASSIGN_OP_MOD: return BIN_OP_MOD;
        case ASSIGN_OP_BIT_OR: return BIN_OP_BIT_OR;
        case ASSIGN_OP_BIT_AND: return BIN_OP_BIT_AND;
        case ASSIGN_OP_BIT_XOR: return BIN_OP_BIT_XOR;
        case ASSIGN_OP_SHL: return BIN_OP_SHL;
        case ASSIGN_OP_SHR: return BIN_OP_SHR;
        default: FABRIC_ASSERT( false ); return BIN_OP_ADD;
      }
    }
  };
};

#endif //_FABRIC_CG_OP_TYPES_H

