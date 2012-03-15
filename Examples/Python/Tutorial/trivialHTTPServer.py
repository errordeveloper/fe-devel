import fabric
import BaseHTTPServer

class FabricHandler( BaseHTTPServer.BaseHTTPRequestHandler ):
  def do_GET( self ):
    client = fabric.createClient()
    self.send_response( 200 )
    self.send_header( 'Content-type', 'text/plain' )
    self.end_headers()
    self.wfile.write( client.build.getName() + ' version ' + client.build.getFullVersion() + '\n' )
    client.close()

port = 1337
httpd = BaseHTTPServer.HTTPServer( ('', port), FabricHandler )
print('Server running at http://127.0.0.1:' + str(port) + '/')
httpd.serve_forever()

