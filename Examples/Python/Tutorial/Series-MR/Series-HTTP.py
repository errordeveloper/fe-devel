import fabric
import BaseHTTPServer
import urlparse

fabricClient = fabric.createClient()
  
# Load the source code and compile into an executable

compilation = fabricClient.KLC.createCompilation()
compilation.addSource('computeTerm.kl', open('computeTerm.kl').read())
compilation.addSource('subTerms.kl', open('sumTerms.kl').read())
executable = compilation.run()

class FabricHandler( BaseHTTPServer.BaseHTTPRequestHandler ):
  def do_GET( self ):
    # Take the number of terms to compute from the 'n' GET
    # parameter default to 10
    numTerms = 10
    query = urlparse.parse_qs(urlparse.urlparse(self.path).query)
    if 'n' in query:
      numTerms = int( query[ 'n' ][ 0 ] )

    # Compute the result using a simple map-reduce
    result = fabricClient.MR.createReduce(
      fabricClient.MR.createArrayGenerator(
        fabricClient.MR.createConstValue('Size', numTerms),
        executable.resolveArrayGeneratorOperator('computeTerm')
        ),
      executable.resolveReduceOperator('sumTerms')
      ).produce()
  
    # Return the result as the HTTP content
 
    self.send_response( 200 )
    self.send_header( 'Content-type', 'text/plain' )
    self.end_headers()
    self.wfile.write( str(result) + '\n' )

# Create the HTTP server
port = 1337
httpd = BaseHTTPServer.HTTPServer( ('', port), FabricHandler )
print('Server running at http://127.0.0.1:' + str(port) + '/')
httpd.serve_forever()

