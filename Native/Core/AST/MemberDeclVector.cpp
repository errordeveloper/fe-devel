/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "MemberDeclVector.h"
#include "MemberDecl.h"
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    RC::ConstHandle<MemberDeclVector> MemberDeclVector::Create( RC::ConstHandle<MemberDecl> const &first, RC::ConstHandle<MemberDeclVector> const &remaining )
    {
      MemberDeclVector *result = new MemberDeclVector;
      if ( first )
        result->push_back( first );
      if ( remaining )
      {
        for ( MemberDeclVector::const_iterator it=remaining->begin(); it!=remaining->end(); ++it )
          result->push_back( *it );
      }
      return result;
    }
    
    MemberDeclVector::MemberDeclVector()
    {
    }
    
    void MemberDeclVector::appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const
    {
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->appendJSON( jsonArrayEncoder.makeElement(), includeLocation );
    }
    
    void MemberDeclVector::buildStructMemberInfoVector( RC::ConstHandle<RT::Manager> const &rtManager, RT::StructMemberInfoVector &structMemberInfoVector ) const
    {
      for ( const_iterator it=begin(); it!=end(); ++it )
      {
        RT::StructMemberInfo structMemberInfo;
        (*it)->buildStructMemberInfo( rtManager, structMemberInfo );
        structMemberInfoVector.push_back( structMemberInfo );
      }
    }
    
    void MemberDeclVector::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->registerTypes( cgManager, diagnostics );
    }
  };
};
