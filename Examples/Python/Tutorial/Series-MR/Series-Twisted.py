import fabric
from twisted.web import server, resource
from twisted.internet import reactor
import urlparse

fabricClient = fabric.createClient()
  
# Load the source code and compile into an executable

compilation = fabricClient.KLC.createCompilation()
compilation.addSource('computeTerm.kl', open('computeTerm.kl').read())
compilation.addSource('subTerms.kl', open('sumTerms.kl').read())
executable = compilation.run()

class FabricResource( resource.Resource ):
  isLeaf = True

  def render_GET( self, request ):
    # Take the number of terms to compute from the 'n' GET
    # parameter default to 10
    numTerms = 10

    if 'n' in request.args:
      numTerms = int( request.args[ 'n' ][ 0 ] )

    # Compute the result using a simple map-reduce
    result = fabricClient.MR.createReduce(
      fabricClient.MR.createArrayGenerator(
        fabricClient.MR.createConstValue('Size', numTerms),
        executable.resolveArrayGeneratorOperator('computeTerm')
        ),
      executable.resolveReduceOperator('sumTerms')
      ).produce()
  
    # Return the result as the HTTP content
 
    request.setHeader( 'Content-type', 'text/plain' )
    return str(result) + '\n'

port = 1337
reactor.listenTCP( port, server.Site( FabricResource() ) )
print('Server running at http://127.0.0.1:' + str(port) + '/')
reactor.run()

