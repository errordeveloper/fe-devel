/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KL/Compiler.h>
#include <Fabric/Core/KL/StringSource.h>
#include <Fabric/Core/KL/Scanner.h>
#include <Fabric/Core/KL/Parser.hpp>
#include <Fabric/Core/CG/Diagnostics.h>

namespace Fabric
{
  namespace KL
  {
    RC::ConstHandle<Compiler> Compiler::Create()
    {
      return new Compiler;
    }
    
    Compiler::Compiler()
    {
    }
    
    RC::ConstHandle<RC::Object> Compiler::compile( std::string const &klFilename, std::string const &klSource ) const
    {
      RC::ConstHandle<Source> source = StringSource::Create( klFilename, klSource );
      RC::Handle<Scanner> scanner = Scanner::Create( source );
      CG::Diagnostics diagnostics;
      RC::ConstHandle<AST::GlobalList> ast = KL::Parse( scanner, diagnostics );
      if ( diagnostics.containsError() )
      {
        std::string firstErrorDesc = "";
        for ( CG::Diagnostics::const_iterator it=diagnostics.begin(); it!=diagnostics.end(); ++it )
        {
          if ( it->second.getLevel() == CG::Diagnostic::LEVEL_ERROR )
          {
            firstErrorDesc = it->first.desc() + ": " + it->second.desc();
            break;
          }
        }
        throw Exception( "KL compile failed: " + firstErrorDesc );
      }
      return ast;
    }
  }
}
