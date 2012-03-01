/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/Diagnostics.h>

namespace Fabric
{
  namespace CG
  {
    Diagnostics::Diagnostics( Diagnostics const &that )
      : m_containsError( false )
    {
      append( that );
    }
    
    void Diagnostics::append( Diagnostics const &that )
    {
      m_containsError = m_containsError || that.m_containsError;
      for ( Impl::const_iterator it = that.m_impl.begin(); it != that.m_impl.end(); ++it )
        m_impl.push_back( *it );
    }
    
    void Diagnostics::encodeJSON( JSON::Encoder &jg ) const
    {
      JSON::ArrayEncoder resultArrayEncoder = jg.makeArray();
      for ( CG::Diagnostics::const_iterator it=begin(); it!=end(); ++it )
      {
        CG::Location const &location = it->first;
        CG::Diagnostic const &diagnostic = it->second;
        
        JSON::ObjectEncoder resultObjectEncoder = resultArrayEncoder.makeElement().makeObject();
        resultObjectEncoder.makeMember( "filename", 8 ).makeString( location.getFilename() );
        resultObjectEncoder.makeMember( "line", 4 ).makeInteger( location.getLine() );
        resultObjectEncoder.makeMember( "column", 6 ).makeInteger( location.getColumn() );
        resultObjectEncoder.makeMember( "level", 5 ).makeString( diagnostic.getLevelDesc() );
        resultObjectEncoder.makeMember( "desc", 4 ).makeString( diagnostic.getDesc().getData(), diagnostic.getDesc().getLength() );
      }
    }
  }
}
