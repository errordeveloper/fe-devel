/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_WINDOWLESS_CG_VIEW_PORT_H
#define _FABRIC_NPAPI_WINDOWLESS_CG_VIEW_PORT_H

#include <Fabric/Clients/NPAPI/ViewPort.h>

#include <npapi/npapi.h>
#include <AGL/agl.h>
#include <vector>

namespace Fabric
{
  namespace NPAPI
  {
    class WindowlessCGViewPort : public ViewPort
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ViewPort> Create( RC::ConstHandle<Interface> const &interface );

      virtual void getWindowSize( size_t &width, size_t &height ) const
      {
        width = m_windowWidth;
        height = m_windowHeight;
      }

      virtual NPError nppSetWindow( NPWindow *npWindow );
      virtual int16_t nppHandleEvent( void *event );

      virtual void needsRedraw();

      virtual void pushOGLContext();
      virtual void popOGLContext();

      virtual std::string queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension );

    protected:
    
      WindowlessCGViewPort( RC::ConstHandle<Interface> const &interface );
      ~WindowlessCGViewPort();
          
    private:
    
      NPP m_npp;
      
      CGContextRef m_cgContext;
      
      WindowRef m_windowRef;
      AGLContext m_windowAGLContext;
      
      int m_windowLeft, m_windowTop;
      
      unsigned int m_windowWidth, m_windowHeight;
      void *m_offscreenData;
      CGImageRef m_offscreenCGImage;
      std::vector<AGLContext> m_aglContextStack;
    };
  };
};

#endif //_FABRIC_NPAPI_WINDOWLESS_CG_VIEW_PORT_H
