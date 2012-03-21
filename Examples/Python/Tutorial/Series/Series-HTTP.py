import fabric
import BaseHTTPServer
import urlparse

class FabricHandler( BaseHTTPServer.BaseHTTPRequestHandler ):
  def do_GET( self ):

    query = urlparse.parse_qs(urlparse.urlparse(self.path).query)
    numTerms = 10
    if 'n' in query:
      numTerms = int( query[ 'n' ][ 0 ] )

    fabricClient = fabric.createClient()

    # Create the operator that computes the value for
    # each term of the series

    computeTermOp = fabricClient.DG.createOperator("computeTermOp")
    computeTermOp.setSourceCode('computeTerm.kl', open('computeTerm.kl').read())
    computeTermOp.setEntryFunctionName('computeTerm')

    # Create the binding that binds the computeTermOp to the
    # terms node.  A binding binds the members of the node
    # to the arguments to the operator

    computeTermBinding = fabricClient.DG.createBinding()
    computeTermBinding.setOperator(computeTermOp)
    computeTermBinding.setParameterLayout([
      "self.index",   # self.index is special: the index of the
                      # slice being operated on
      "self.result"
    ])

    # Create the node that holds the terms in the series.
    # The number of terms is the "count" of the node,
    # ie. the SIMD multiplicity

    termsNode = fabricClient.DG.createNode("termsNode")
    termsNode.setCount(numTerms)
    termsNode.addMember("result", "Scalar")
    termsNode.bindings.append(computeTermBinding)

    # Create the operator that sums the terms of the series

    sumTermsOp = fabricClient.DG.createOperator("sumTermsOp")
    sumTermsOp.setSourceCode('sumTerms.kl', open('sumTerms.kl').read())
    sumTermsOp.setEntryFunctionName('sumTerms')

    # Create the binding that binds sumTermsOp to the members of
    # sumNode

    sumTermsBinding = fabricClient.DG.createBinding()
    sumTermsBinding.setOperator(sumTermsOp)
    sumTermsBinding.setParameterLayout([
      "terms",            # terms is special: it is an object that
                          # allows you to get and set the number of slices
      "terms.result<>",   # the <> syntax specifies that we want to bind
                          # to all the slices at once
      "self.result" 
    ])

    # Create the node to hold the result, add termsNode as a
    # dependency and append the binding for sumTermsOp

    sumNode = fabricClient.DG.createNode("sumNode")
    sumNode.addMember("result", "Scalar")
    sumNode.setDependency(termsNode, "terms")
    sumNode.bindings.append(sumTermsBinding)

    def callback():
      # Return the result as the HTTP content

      self.send_response( 200 )
      self.send_header( 'Content-type', 'text/plain' )
      self.end_headers()
      self.wfile.write( str(sumNode.getData('result', 0)) + '\n' )

      # Close the Fabric Engine client.  If the client isn't closed
      # then Fabric Python client will keep this script alive!

      fabricClient.close()

    # Evaluate the sumNode asynchronously.  The given
    # callback is issue when the computation is done.

    sumNode.evaluateAsync( callback )

# Create the HTTP server
port = 1337
httpd = BaseHTTPServer.HTTPServer( ('', port), FabricHandler )
print('Server running at http://127.0.0.1:' + str(port) + '/')
httpd.serve_forever()

