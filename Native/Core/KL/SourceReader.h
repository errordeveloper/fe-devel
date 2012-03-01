/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_SOURCE_READER_H
#define _FABRIC_KL_SOURCE_READER_H

#include <Fabric/Core/KL/Source.h>
#include <Fabric/Core/KL/Location.h>
#include <Fabric/Base/Util/Assert.h>

namespace Fabric
{
  namespace KL
  {
    class SourceReader
    {
    public:
    
      SourceReader( RC::ConstHandle<Source> const &source )
        : m_source( source )
        , m_data( source->data() )
        , m_length( source->length() )
        , m_location( source->filename() )
      {
      }
    
      SourceReader( RC::ConstHandle<Source> const &source, Location const &initialLocation )
        : m_source( source )
        , m_data( source->data() )
        , m_length( source->length() )
        , m_location( initialLocation )
      {
      }
      
      RC::ConstHandle<Source> getSource() const
      {
        return m_source;
      }
    
      Location getLocationForStart()
      {
        forwardThroughEscapes();
        return m_location;
      }
      
      Location getLocationForEnd() const
      {
        return m_location;
      }
      
      bool isEOF()
      {
        forwardThroughEscapes();
        return m_location.getIndex() == m_length;
      }
      
      int peek()
      {
        forwardThroughEscapes();
        size_t index = m_location.getIndex();
        if ( index < m_length )
          return m_data[index];
        else return -1;
      }
      
      char advance()
      {
        forwardThroughEscapes();
        FABRIC_ASSERT( m_location.getIndex() < m_length );
        char result = m_data[m_location.getIndex()];

        if ( result == '\n' )
        {
          m_location.setLine( m_location.getLine() + 1 );
          m_location.setColumn( 1 );
        }
        else 
        {
          m_location.setColumn( m_location.getColumn() + 1 );
        }
        m_location.setIndex( m_location.getIndex() + 1 );
        return result;
      }
    
      void forwardThroughEscapes()
      {
        for (;;)
        {
          // [pzion 20110715] UNIX line endings
          if ( m_location.getIndex()+1 < m_length
            && m_data[m_location.getIndex()] == '\\'
            && m_data[m_location.getIndex()+1] == '\n' )
          {
            m_location.setIndex( m_location.getIndex() + 2 );
            m_location.setLine( m_location.getLine() + 1 );
            m_location.setColumn( 1 );
          }
          // [pzion 20110715] DOS line endings
          else if ( m_location.getIndex()+2 < m_length 
            && m_data[m_location.getIndex()] == '\\'
            && m_data[m_location.getIndex()+1] == '\r'
            && m_data[m_location.getIndex()+2] == '\n' )
          {
            m_location.setIndex( m_location.getIndex() + 3 );
            m_location.setLine( m_location.getLine() + 1 );
            m_location.setColumn( 1 );
          }
          else if ( m_location.getIndex() + 1 == m_length
            && m_data[m_location.getIndex()] == '\\' )
          {
            m_location.setIndex( m_location.getIndex() + 1 );
            m_location.setColumn( m_location.getColumn() + 1 );
          }
          else
          {
            break;
          }
        }
      }
    
    private:
      
      RC::ConstHandle<Source> m_source;
      char const *m_data;
      size_t m_length;
      Location m_location;
    };
  };
};

#endif //_FABRIC_KL_SOURCE_READER_H
