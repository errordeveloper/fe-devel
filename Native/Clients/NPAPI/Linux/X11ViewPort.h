/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_X11_VIEW_PORT_H
#define _FABRIC_NPAPI_X11_VIEW_PORT_H

#include <Fabric/Clients/NPAPI/ViewPort.h>

#include <npapi/npapi.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <vector>

namespace Fabric
{
  namespace DG
  {
    class Context;
  };

  namespace NPAPI
  {
    class X11ViewPort : public ViewPort
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ViewPort> Create( RC::ConstHandle<Interface> const &interface );
      
      virtual void getWindowSize( size_t &width, size_t &height ) const
      {
        width = m_windowWidth;
        height = m_windowHeight;
      }

      virtual void needsRedraw();
      
      virtual NPError nppGetValue( NPPVariable variable, void *value );
      virtual NPError nppSetWindow( NPWindow *window );

      virtual void pushOGLContext();
      virtual void popOGLContext();
      
      virtual std::string queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension );

    protected:
    
      X11ViewPort( RC::ConstHandle<Interface> const &interface );
      ~X11ViewPort();
          
    private:

      void redraw();
      static gboolean EventCallback( GtkWidget *widget, GdkEvent *event, gpointer user_data );
      static void MenuItemActivateCallback( void *_popUpItem );
      
      RC::Handle<MT::LogCollector> m_logCollector;
      GdkNativeWindow m_gdkNativeWindow;
      GtkWidget *m_plug;
      GtkWidget *m_drawingArea;
      int m_windowLeft, m_windowTop;
      unsigned m_windowWidth, m_windowHeight;
      typedef std::pair< GdkGLDrawable *, GdkGLContext * > GdkGLDrawableAndContext;
      std::vector<GdkGLDrawableAndContext> m_gdkGLStack;
    };
  };
};

#endif //_FABRIC_NPAPI_X11_VIEW_PORT_H
