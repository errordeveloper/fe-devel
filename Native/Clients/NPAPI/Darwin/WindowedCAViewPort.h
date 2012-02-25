/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_WINDOWED_CA_VIEW_PORT_H
#define _FABRIC_NPAPI_WINDOWED_CA_VIEW_PORT_H

#include <Fabric/Clients/NPAPI/ViewPort.h>

#include <npapi/npapi.h>
#include <vector>

#ifdef __OBJC__
@class CALayer;
@class NPCAOpenGLLayer;
#else
class CALayer;
class NPCAOpenGLLayer;
#endif

namespace Fabric
{
  namespace NPAPI
  {
    class WindowedCAViewPort : public ViewPort
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ViewPort> Create( RC::ConstHandle<Interface> const &interface );

      virtual void getWindowSize( size_t &width, size_t &height ) const;
      void setWindowSize( size_t width, size_t height );
    
      virtual NPError nppGetValue( NPPVariable variable, void *value );
      virtual int16_t nppHandleEvent( void *event );
      virtual NPError nppDestroy( NPSavedData** save );

      virtual void needsRedraw();
      virtual void redrawFinished();

      virtual void pushOGLContext();
      virtual void popOGLContext();

      virtual std::string queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension );

    protected:
    
      WindowedCAViewPort( RC::ConstHandle<Interface> const &interface );
      ~WindowedCAViewPort();

      //virtual void timerFired();
      std::string getPathFromSaveAsDialog( std::string const &title, std::string const &defaultFilename, std::string const &extension );
      std::string getPathFromOpenDialog( std::string const &title, std::string const &extension );

    private:

      NPP m_npp;
      NPCAOpenGLLayer *m_npCAOpenGLLayer;
      size_t m_width, m_height;
      std::vector<CGLContextObj> m_cglContextStack;
    };
  };
};

#endif //_FABRIC_NPAPI_WINDOWED_CA_VIEW_PORT_H
