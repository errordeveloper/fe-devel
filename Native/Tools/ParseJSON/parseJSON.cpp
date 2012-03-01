/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Assert.h>

#include <stdio.h>
#include <string>
#include <iostream>

void displayEntity( Fabric::JSON::Entity const &entity, std::string const &indent )
{
  std::cout << indent;
  
  switch ( entity.type )
  {
    case Fabric::JSON::Entity::ET_NULL:
      std::cout << "NULL" << std::endl;
      break;
    case Fabric::JSON::Entity::ET_BOOLEAN:
      std::cout << "BOOLEAN ";
      std::cout << (entity.value.boolean? "true": "false");
      std::cout << std::endl;
      break;
    case Fabric::JSON::Entity::ET_INTEGER:
      std::cout << "INTEGER ";
      std::cout << Fabric::_(entity.value.integer);
      std::cout << std::endl;
      break;
    case Fabric::JSON::Entity::ET_SCALAR:
      std::cout << "SCALAR ";
      std::cout << Fabric::_(entity.value.scalar);
      std::cout << std::endl;
      break;
    case Fabric::JSON::Entity::ET_STRING:
      std::cout << "STRING ";
      std::cout << entity.value.string.length;
      std::cout << " ";
      std::cout << Fabric::_( entity.value.string.shortData, entity.value.string.length, Fabric::JSON::DecoderShortStringMaxLength );
      std::cout << std::endl;
      break;
    case Fabric::JSON::Entity::ET_OBJECT:
    {
      std::cout << "OBJECT " << entity.value.object.size << std::endl;
      Fabric::JSON::ObjectDecoder objectParser( entity );
      Fabric::JSON::Entity keyEntityInfo, valueEntityInfo;
      while ( objectParser.getNext( keyEntityInfo, valueEntityInfo ) )
      {
        displayEntity( keyEntityInfo, "  " + indent );
        displayEntity( valueEntityInfo, "    " + indent );
      }
    }
    break;
    case Fabric::JSON::Entity::ET_ARRAY:
    {
      std::cout << "ARRAY " << entity.value.array.size << std::endl;
      Fabric::JSON::ArrayDecoder arrayParser( entity );
      Fabric::JSON::Entity elementEntityInfo;
      while ( arrayParser.getNext( elementEntityInfo ) )
        displayEntity( elementEntityInfo, "  " + indent );
    }
    break;
    
    default:
      FABRIC_ASSERT( false );
      break;
  }
}

void parseJSON( FILE *fp )
{
  static const size_t maxLength = 16*1024*1024;
  char *data = new char[maxLength];
  size_t length = 0;
  while ( !feof( fp ) )
  {
    int read = fread( &data[length], 1, maxLength - length, fp );
    if ( read <= 0 )
      break;
    length += read;
  }
  
  Fabric::JSON::Decoder decoder( data, length );
  Fabric::JSON::Entity entity;
  while ( decoder.getNext( entity ) )
    displayEntity( entity, "" );
    
  delete [] data;
}

int main( int argc, char **argv )
{
  if ( argc == 1 )
    parseJSON( stdin );
  else
  {
    for ( int i=1; i<argc; ++i )
    {
      FILE *fp = fopen( argv[i], "r" );
      if ( !fp )
        perror( argv[i] );
      parseJSON( fp );
      fclose( fp );
    }
  }
  return 0;
}
