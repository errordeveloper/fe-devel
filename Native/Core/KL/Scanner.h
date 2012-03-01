/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_SCANNER_H
#define _FABRIC_KL_SCANNER_H

#include <Fabric/Core/KL/Token.h>
#include <Fabric/Core/KL/SourceReader.h>

namespace Fabric
{
  namespace KL
  {
    class Scanner : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Scanner> Create( RC::ConstHandle<Source> const &source )
      {
        return new Scanner( source );
      }
      
      SourceReader const &getSourceReader() const
      {
        return m_sourceReader;
      }
      
      Token nextToken();
    
      Location getLocationForStart()
      {
        return m_sourceReader.getLocationForStart();
      }
      
      Location getLocationForEnd() const
      {
        return m_sourceReader.getLocationForEnd();
      }
      
    protected:
    
      Scanner( RC::ConstHandle<Source> const &source );
    
      SourceRange createSourceRange( Location const &startLocation ) const;
      Token createToken( Token::Type tokenType, SourceRange const &sourceRange ) const;
      Token createToken( Token::Type tokenType, Location const &startLocation ) const
      {
        return createToken( tokenType, createSourceRange( startLocation ) );
      }
    
      Token nextToken_Alpha( Location const &startLocation );
      Token nextToken_Digit( Location const &startLocation );
      Token nextToken_Space( Location const &startLocation );
      Token nextToken_Symbol( Location const &startLocation );
    
      static bool IsAlpha( int ch )
      {
        return ch != -1 && s_charAttribs[uint8_t(ch)] & k_charAttribAlpha;
      }
      
      static bool IsDigit( int ch )
      {
        return ch != -1 && s_charAttribs[uint8_t(ch)] & k_charAttribDigit;
      }
      
      static bool IsHexDigit( int ch )
      {
        return ch != -1 && s_charAttribs[uint8_t(ch)] & k_charAttribHexDigit;
      }
      
      static bool IsSpace( int ch )
      {
        return ch != -1 && s_charAttribs[uint8_t(ch)] & k_charAttribSpace;
      }
      
      static bool IsSymbol( int ch )
      {
        return ch != -1 && s_charAttribs[uint8_t(ch)] & k_charAttribSymbol;
      }
      
    private:
    
      SourceReader m_sourceReader;

      static bool s_charAttribsInitialized;
      static uint8_t s_charAttribs[256];
      static const uint8_t k_charAttribAlpha = 1;
      static const uint8_t k_charAttribDigit = 2;
      static const uint8_t k_charAttribSpace = 4;
      static const uint8_t k_charAttribSymbol = 8;
      static const uint8_t k_charAttribHexDigit = 16;
    };
  };
};

#endif //_FABRIC_KL_BUFFER_H
