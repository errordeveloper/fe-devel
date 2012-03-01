/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Debug.h"
#include <stdint.h>

namespace Fabric
{
  namespace Util
  {
#if defined(FABRIC_BUILD_DEBUG)
    bool gDebugEnabled = false;

    TLSVar<unsigned> Trace::m_indent;
    char **Trace::m_indentStrings;

    void Trace::dumpBuffer( unsigned size, void const *_data )
    {
      static char hexDigits[] = "0123456789ABCDEF";
      uint8_t const *data = (uint8_t const *)_data;
      unsigned offset = 0;
      while ( offset < size )
      {
        char buf[16*3+1+16*1+1];
        buf[16*3] = ' ';
        buf[16*3+1+16*1] = '\0';
        for ( unsigned i=0; i<16; ++i )
        {
          if ( offset+i < size )
          {
            buf[3*i+0] = hexDigits[data[offset+i]>>4];
            buf[3*i+1] = hexDigits[data[offset+i]&0x0Fu];
            buf[16*3+1+i] = isprint(data[offset+i])? data[offset+i]: '_';
          }
          else buf[3*i+0] = buf[3*i+1] = buf[16*3+1+i] = ' ';
          
          if ( i == 7 && offset+i+1<size )
            buf[3*i+2] = '-';
          else buf[3*i+2] = ' ';
        }
        note( "%s", buf );
        offset += 16;
      }
    }
#endif
  }
}
