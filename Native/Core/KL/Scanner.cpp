/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KL/Scanner.h>
#include <Fabric/Core/KL/Source.h>
#include <Fabric/Core/KL/Location.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/CG/Manager.h>

namespace Fabric
{
  namespace KL
  {
    bool Scanner::s_charAttribsInitialized = false;
    uint8_t Scanner::s_charAttribs[256];

    Scanner::Scanner( RC::ConstHandle<Source> const &source )
      : m_sourceReader( source )
    {
      if ( !s_charAttribsInitialized )
      {
        memset( s_charAttribs, 0, 256 );
        
        for ( size_t ch='a'; ch<='z'; ++ch )
          s_charAttribs[ch] |= k_charAttribAlpha;
        for ( size_t ch='A'; ch<='Z'; ++ch )
          s_charAttribs[ch] |= k_charAttribAlpha;
        for ( size_t ch=128; ch<256; ++ch )
          s_charAttribs[ch] |= k_charAttribAlpha;        
        s_charAttribs['_'] |= k_charAttribAlpha;
        
        for ( size_t ch='0'; ch<='9'; ++ch )
          s_charAttribs[ch] |= k_charAttribDigit;
        
        for ( size_t ch='0'; ch<='9'; ++ch )
          s_charAttribs[ch] |= k_charAttribHexDigit;
        for ( size_t ch='a'; ch<='f'; ++ch )
          s_charAttribs[ch] |= k_charAttribHexDigit;
        for ( size_t ch='A'; ch<='F'; ++ch )
          s_charAttribs[ch] |= k_charAttribHexDigit;
          
        s_charAttribs[' '] |= k_charAttribSpace;
        s_charAttribs['\n'] |= k_charAttribSpace;
        s_charAttribs['\r'] |= k_charAttribSpace;
        s_charAttribs['\t'] |= k_charAttribSpace;
        s_charAttribs['\v'] |= k_charAttribSpace;
        
        s_charAttribs['~'] |= k_charAttribSymbol;
        s_charAttribs['`'] |= k_charAttribSymbol;
        s_charAttribs['!'] |= k_charAttribSymbol;
        s_charAttribs['@'] |= k_charAttribSymbol;
        s_charAttribs['#'] |= k_charAttribSymbol;
        s_charAttribs['$'] |= k_charAttribSymbol;
        s_charAttribs['%'] |= k_charAttribSymbol;
        s_charAttribs['^'] |= k_charAttribSymbol;
        s_charAttribs['&'] |= k_charAttribSymbol;
        s_charAttribs['*'] |= k_charAttribSymbol;
        s_charAttribs['('] |= k_charAttribSymbol;
        s_charAttribs[')'] |= k_charAttribSymbol;
        s_charAttribs['-'] |= k_charAttribSymbol;
        s_charAttribs['='] |= k_charAttribSymbol;
        s_charAttribs['+'] |= k_charAttribSymbol;
        s_charAttribs['|'] |= k_charAttribSymbol;
        s_charAttribs[']'] |= k_charAttribSymbol;
        s_charAttribs['}'] |= k_charAttribSymbol;
        s_charAttribs['['] |= k_charAttribSymbol;
        s_charAttribs['{'] |= k_charAttribSymbol;
        s_charAttribs[';'] |= k_charAttribSymbol;
        s_charAttribs[':'] |= k_charAttribSymbol;
        s_charAttribs['\''] |= k_charAttribSymbol;
        s_charAttribs['"'] |= k_charAttribSymbol;
        s_charAttribs['/'] |= k_charAttribSymbol;
        s_charAttribs['?'] |= k_charAttribSymbol;
        s_charAttribs['.'] |= k_charAttribSymbol;
        s_charAttribs['>'] |= k_charAttribSymbol;
        s_charAttribs[','] |= k_charAttribSymbol;
        s_charAttribs['<'] |= k_charAttribSymbol;
        
        s_charAttribsInitialized = true;
      }
    }
    
    Token Scanner::nextToken_Symbol( Location const &startLocation )
    {
      FABRIC_ASSERT( IsSymbol( m_sourceReader.peek() ) );
      char ch = m_sourceReader.advance();
      switch ( ch )
      {
        case '=':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_EQUALS_EQUALS, startLocation );
          }
          else return createToken( TOKEN_EQUALS, startLocation );        
           
        case '+':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_PLUS_EQUALS, startLocation );
          }
          else if ( m_sourceReader.peek() == '+' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_PLUS_PLUS, startLocation );
          }
          else return createToken( TOKEN_PLUS, startLocation );        
        
        case '-':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_MINUS_EQUALS, startLocation );
          }
          if ( m_sourceReader.peek() == '-' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_MINUS_MINUS, startLocation );
          }
          else return createToken( TOKEN_MINUS, startLocation );
          
        case '*':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_AST_EQUALS, startLocation );
          }
          else return createToken( TOKEN_AST, startLocation );
          
        case '/':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_FSLASH_EQUALS, startLocation );
          }
          else if ( m_sourceReader.peek() == '*' )
          {
            m_sourceReader.advance();
            while ( !m_sourceReader.isEOF() )
            {
              if ( m_sourceReader.advance() == '*' )
              {
                if ( m_sourceReader.peek() == '/' )
                {
                  m_sourceReader.advance();
                  return createToken( TOKEN_COMMENT_BLOCK, startLocation );
                }
              }
            }
            throw Exception("unclosed block comment");
          }
          else if ( m_sourceReader.peek() == '/' )
          {
            m_sourceReader.advance();
            while ( m_sourceReader.peek() != '\n' )
              m_sourceReader.advance();
            return createToken( TOKEN_COMMENT_LINE, startLocation );
          }
          else return createToken( TOKEN_FSLASH, startLocation );
          
        case '%':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_PERCENT_EQUALS, startLocation );
          }
          else return createToken( TOKEN_PERCENT, startLocation );
          
        case '^':
          if ( m_sourceReader.peek() == '^' )
          {
            m_sourceReader.advance();
            if ( m_sourceReader.peek() == '=' )
            {
              m_sourceReader.advance();
              return createToken( TOKEN_CARET_CARET_EQUALS, startLocation );
            }
            else return createToken( TOKEN_CARET_CARET, startLocation );
          }
          else if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_CARET_EQUALS, startLocation );
          }
          else return createToken( TOKEN_CARET, startLocation );
          
        case '&':
          if ( m_sourceReader.peek() == '&' )
          {
            m_sourceReader.advance();
            if ( m_sourceReader.peek() == '=' )
            {
              m_sourceReader.advance();
              return createToken( TOKEN_AMP_AMP_EQUALS, startLocation );
            }
            else return createToken( TOKEN_AMP_AMP, startLocation );
          }
          else if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_AMP_EQUALS, startLocation );
          }
          else return createToken( TOKEN_AMP, startLocation );
          
        case '|':
          if ( m_sourceReader.peek() == '|' )
          {
            m_sourceReader.advance();
            if ( m_sourceReader.peek() == '=' )
            {
              m_sourceReader.advance();
              return createToken( TOKEN_PIPE_PIPE_EQUALS, startLocation );
            }
            else return createToken( TOKEN_PIPE_PIPE, startLocation );
          }
          else if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_PIPE_EQUALS, startLocation );
          }
          else return createToken( TOKEN_PIPE, startLocation );
          
        case '[':
          return createToken( TOKEN_LBRACKET, startLocation );

        case ']':
          return createToken( TOKEN_RBRACKET, startLocation );
        
        case '{':
          return createToken( TOKEN_LBRACE, startLocation );
          
        case '}':
          return createToken( TOKEN_RBRACE, startLocation );
        
        case '(':
          return createToken( TOKEN_LPAREN, startLocation );
          
        case ')':
          return createToken( TOKEN_RPAREN, startLocation );
        
        case '.':
          return createToken( TOKEN_DOT, startLocation );
        
        case ';':
          return createToken( TOKEN_SEMICOLON, startLocation );
          
        case '<':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_LANGLE_EQUALS, startLocation );
          }
          if ( m_sourceReader.peek() == '<' )
          {
            m_sourceReader.advance();
            if ( m_sourceReader.peek() == '=' )
            {
              m_sourceReader.advance();
              return createToken( TOKEN_LANGLE_LANGLE_EQUALS, startLocation );
            } else return createToken( TOKEN_LANGLE_LANGLE, startLocation );
          }
          else return createToken( TOKEN_LANGLE, startLocation );
          
        case '>':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_RANGLE_EQUALS, startLocation );
          }
          if ( m_sourceReader.peek() == '>' )
          {
            m_sourceReader.advance();
            if ( m_sourceReader.peek() == '=' )
            {
              m_sourceReader.advance();
              return createToken( TOKEN_RANGLE_RANGLE_EQUALS, startLocation );
            } else return createToken( TOKEN_RANGLE_RANGLE, startLocation );
          }
          else return createToken( TOKEN_RANGLE, startLocation );
        
        case '~':
          return createToken( TOKEN_TILDE, startLocation );
        
        case '!':
          if ( m_sourceReader.peek() == '=' )
          {
            m_sourceReader.advance();
            return createToken( TOKEN_EXCL_EQUALS, startLocation );
          }
          else return createToken( TOKEN_EXCL, startLocation );
          
        case '\'':
          while ( !m_sourceReader.isEOF() )
          {
            char ch = m_sourceReader.advance();
            if ( ch == '\\' )
            {
              if ( m_sourceReader.isEOF() )
                throw Exception("unterminated string constant");
              m_sourceReader.advance();
            }
            else if ( ch == '\'' )
            {
              return createToken( TOKEN_CONST_STRING_SQUOT, startLocation );
            }
          }
          throw Exception("unterminated string constant");
          
        case '"':
          while ( !m_sourceReader.isEOF() )
          {
            char ch = m_sourceReader.advance();
            if ( ch == '\\' )
            {
              if ( m_sourceReader.isEOF() )
                throw Exception("unterminated string constant");
              m_sourceReader.advance();
            }
            else if ( ch == '"' )
            {
              return createToken( TOKEN_CONST_STRING_DQUOT, startLocation );
            }
          }
          throw Exception("unterminated string constant");
        
        case ',':
          return createToken( TOKEN_COMMA, startLocation );
          
        case '?':
          return createToken( TOKEN_QUESTION_MARK, startLocation );
        
        case ':':
          return createToken( TOKEN_COLON, startLocation );
          
        default:
          throw Exception("bad symbol");
      }
    }
    
    Token Scanner::nextToken_Digit( Location const &startLocation )
    {
      FABRIC_ASSERT( IsDigit( m_sourceReader.peek() ) );
      
      char ch = m_sourceReader.advance();
      
      Token::Type tokenType = TOKEN_CONST_UI;
      
      if ( ch == '0' && tolower( m_sourceReader.peek() ) == 'x' )
      {
        m_sourceReader.advance();
        if ( !IsHexDigit( m_sourceReader.peek() ) )
          throw Exception("malformed hexadecimal constant");
        while ( IsHexDigit( m_sourceReader.peek() ) )
          m_sourceReader.advance();
      }
      else
      {
        while ( IsDigit( m_sourceReader.peek() ) )
          m_sourceReader.advance();
        
        if ( m_sourceReader.peek() == '.' )
        {
          m_sourceReader.advance();
          tokenType = TOKEN_CONST_FP;
          
          while ( IsDigit( m_sourceReader.peek() ) )
            m_sourceReader.advance();
        }
        
        if ( m_sourceReader.peek() == 'e' || m_sourceReader.peek() == 'E' )
        {
          m_sourceReader.advance();
          tokenType = TOKEN_CONST_FP;
          
          if ( m_sourceReader.peek() == '+' || m_sourceReader.peek() == '-' )
            m_sourceReader.advance();
          
          if ( !IsDigit( m_sourceReader.peek() ) )
            throw Exception("malformed floating point exponent");
          while ( IsDigit( m_sourceReader.peek() ) )
            m_sourceReader.advance();
        }
      }
      
      return createToken( tokenType, startLocation );
    }
    
    Token Scanner::nextToken_Space( Location const &startLocation )
    {
      FABRIC_ASSERT( IsSpace( m_sourceReader.peek() ) );
      while ( IsSpace( m_sourceReader.peek() ) )
        m_sourceReader.advance();
      return createToken( TOKEN_WHITESPACE, startLocation );
    }
    
    Token Scanner::nextToken_Alpha( Location const &startLocation )
    {
      FABRIC_ASSERT( IsAlpha( m_sourceReader.peek() ) );

      m_sourceReader.advance();
      
      while ( IsAlpha( m_sourceReader.peek() ) || IsDigit( m_sourceReader.peek() ) )
        m_sourceReader.advance();
        
      SourceRange sourceRange = createSourceRange( startLocation );
      std::string string = sourceRange.toString();
      switch( string.length() )
      {
        case 2:
          if ( string == "in" )
            return createToken( TOKEN_IN, sourceRange );
          if ( string == "io" )
            return createToken( TOKEN_IO, sourceRange );
          if ( string == "if" )
            return createToken( TOKEN_IF, sourceRange );
          if ( string == "do" )
            return createToken( TOKEN_DO, sourceRange );
          break;
          
        case 3:
          if ( string == "for" )
            return createToken( TOKEN_FOR, sourceRange );
          if ( string == "use" )
            return createToken( TOKEN_USE, sourceRange );
          if ( string == "var" )
            return createToken( TOKEN_VAR, sourceRange );
          break;
          
        case 4:
          if ( string == "case" )
            return createToken( TOKEN_CASE, sourceRange );
          if ( string == "else" )
            return createToken( TOKEN_ELSE, sourceRange );
          if ( string == "true" )
            return createToken( TOKEN_TRUE, sourceRange );
          break;
          
        case 5:
          if ( string == "alias" )
            return createToken( TOKEN_ALIAS, sourceRange );
          if ( string == "break" )
            return createToken( TOKEN_BREAK, sourceRange );
          if ( string == "const" )
            return createToken( TOKEN_CONST, sourceRange );
          if ( string == "false" )
            return createToken( TOKEN_FALSE, sourceRange );
          if ( string == "while" )
            return createToken( TOKEN_WHILE, sourceRange );
          break;
        
        case 6:
          if ( string == "struct" )
            return createToken( TOKEN_STRUCT, sourceRange );
          if ( string == "switch" )
            return createToken( TOKEN_SWITCH, sourceRange );
          if ( string == "report" )
            return createToken( TOKEN_REPORT, sourceRange );
          if ( string == "return" )
            return createToken( TOKEN_RETURN, sourceRange );
          break;
        
        case 7:
          if ( string == "default" )
            return createToken( TOKEN_DEFAULT, sourceRange );
          if ( string == "require" )
            return createToken( TOKEN_REQUIRE, sourceRange );
          break;
        
        case 8:
          if ( string == "function" )
            return createToken( TOKEN_FUNCTION, sourceRange );
          if ( string == "operator" )
            return createToken( TOKEN_OPERATOR, sourceRange );
          if ( string == "continue" )
            return createToken( TOKEN_CONTINUE, sourceRange );
          break;
        
        case 9:
          break;
          
        case 12:
          if ( string == "createReduce" )
            return createToken( TOKEN_CREATE_REDUCE, sourceRange );
          break;
          
        case 13:
          if ( string == "ValueProducer" )
            return createToken( TOKEN_VALUE_PRODUCER, sourceRange );
          if ( string == "ArrayProducer" )
            return createToken( TOKEN_ARRAY_PRODUCER, sourceRange );
          break;
        
        case 14:
          if ( string == "createValueMap" )
            return createToken( TOKEN_CREATE_VALUE_MAP, sourceRange );
          if ( string == "createArrayMap" )
            return createToken( TOKEN_CREATE_ARRAY_MAP, sourceRange );
          break;
        
        case 16:
          if ( string == "createConstValue" )
            return createToken( TOKEN_CREATE_CONST_VALUE, sourceRange );
          if ( string == "createConstArray" )
            return createToken( TOKEN_CREATE_CONST_ARRAY, sourceRange );
          if ( string == "createValueCache" )
            return createToken( TOKEN_CREATE_VALUE_CACHE, sourceRange );
          if ( string == "createArrayCache" )
            return createToken( TOKEN_CREATE_ARRAY_CACHE, sourceRange );
          break;
        
        case 20:
          if ( string == "createArrayGenerator" )
            return createToken( TOKEN_CREATE_ARRAY_GENERATOR, sourceRange );
          if ( string == "createArrayTransform" )
            return createToken( TOKEN_CREATE_ARRAY_TRANSFORM, sourceRange );
          if ( string == "createValueGenerator" )
            return createToken( TOKEN_CREATE_VALUE_GENERATOR, sourceRange );
          if ( string == "createValueTransform" )
            return createToken( TOKEN_CREATE_VALUE_TRANSFORM, sourceRange );
          break;
      }

      return createToken( TOKEN_IDENTIFIER, sourceRange );
    }
    
    Token Scanner::nextToken()
    {
      int ch = m_sourceReader.peek();
      Location startLocation = m_sourceReader.getLocationForStart();
      if ( ch == -1 )
        return createToken( TOKEN_END, startLocation );
      if ( IsSpace(ch) )
        return nextToken_Space( startLocation );
      if ( IsAlpha(ch) )
        return nextToken_Alpha( startLocation );
      if ( IsDigit(ch) )
        return nextToken_Digit( startLocation );
      if ( IsSymbol(ch) )
        return nextToken_Symbol( startLocation );
      throw Exception( "bad character" );
    }

    SourceRange Scanner::createSourceRange( Location const &startLocation ) const
    {
      return SourceRange( m_sourceReader.getSource(), startLocation, m_sourceReader.getLocationForEnd() );
    }

    Token Scanner::createToken( Token::Type tokenType, SourceRange const &sourceRange ) const
    {
      return Token( sourceRange, tokenType );
    }
  };
};
