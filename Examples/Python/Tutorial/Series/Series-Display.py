import fabric
import sys

fabricClient = fabric.createClient()

# Take the number of terms to compute on the command
# line default to 10

numTerms = 10
if len( sys.argv ) > 1:
  numTerms = int( sys.argv[1] )

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
                      # allows you to get and set the number of
                      # slices of the node
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

# Evaluate the sumNode (which evalutes its dependecy, the
# termsNode, first) and then print the result.

errors = sumNode.getErrors()
for error in errors:
  print error
sumNode.evaluate()
print(sumNode.getData('result', 0))

# Close the Fabric Engine client.  If the client isn't closed
# then the Python module will keep this script alive!

fabricClient.close()
