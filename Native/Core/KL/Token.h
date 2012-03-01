/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_TOKEN_H
#define _FABRIC_KL_TOKEN_H

#include <Fabric/Core/KL/SourceRange.h>
#include <Fabric/Core/KL/Parser.hpp>

namespace Fabric
{
  namespace KL
  {
    class Token
    {
    public:
    
      typedef enum yytokentype Type;
      
      Token( SourceRange const &sourceRange, Type type = TOKEN_END )
        : m_type( type )
        , m_sourceRange( sourceRange )
      {
      }
      
      Token( RC::ConstHandle<Source> const &source, Location const &startLocation, Location const &endLocation, Type type )
        : m_type( type )
        , m_sourceRange( source, startLocation, endLocation )
      {
      }
      
      Token( Token const &that )
        : m_type( that.m_type )
        , m_sourceRange( that.m_sourceRange )
      {
      }
      
      Token &operator =( Token const &that )
      {
        m_type = that.m_type;
        m_sourceRange = that.m_sourceRange;
        return *this;
      }
      
      Type getType() const
      {
        return m_type;
      }
      
      SourceRange const &getSourceRange() const
      {
        return m_sourceRange;
      }
      
      Location const &getStartLocation() const
      {
        return m_sourceRange.getStartLocation();
      }
      
      Location const &getEndLocation() const
      {
        return m_sourceRange.getEndLocation();
      }
      
      std::string toString() const
      {
        return m_sourceRange.toString();
      }
    
    private:
    
      Type m_type;
      SourceRange m_sourceRange;
    };
  };
};

#endif //_FABRIC_KL_TOKEN_H
