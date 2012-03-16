var fabricClient = require('Fabric').createClient();
var fs = require('fs');

// Take the number of terms to compute on the command
// line; default to 10

var numTerms = parseInt(process.argv[2]) || 10;

// Create the operator that computes the value for
// each term of the series

var computeTermOp = fabricClient.DG.createOperator("computeTermOp");
computeTermOp.setSourceCode('computeTerm.kl', fs.readFileSync('computeTerm.kl', 'utf8'));
computeTermOp.setEntryFunctionName('computeTerm');

// Create the binding that binds the computeTermOp to the
// terms node.  A binding binds the members of the node
// to the arguments to the operator

var computeTermBinding = fabricClient.DG.createBinding();
computeTermBinding.setOperator(computeTermOp);
computeTermBinding.setParameterLayout([
  "self.index",   // self.index is special: the index of the
                  // slice being operated on
  "self.result"
]);

// Create the node that holds the terms in the series.
// The number of terms is the "count" of the node,
// ie. the SIMD multiplicity

var termsNode = fabricClient.DG.createNode("termsNode");
termsNode.setCount(numTerms);
termsNode.addMember("result", "Scalar");
termsNode.bindings.append(computeTermBinding);

// Create the operator that sums the terms of the series

var sumTermsOp = fabricClient.DG.createOperator("sumTermsOp");
sumTermsOp.setSourceCode('sumTerms.kl', fs.readFileSync('sumTerms.kl', 'utf8'));
sumTermsOp.setEntryFunctionName('sumTerms');

// Create the binding that binds sumTermsOp to the members of
// sumNode

var sumTermsBinding = fabricClient.DG.createBinding();
sumTermsBinding.setOperator(sumTermsOp);
sumTermsBinding.setParameterLayout([
  "terms.count",      // terms.count is special: the slice count of
                      // the dependency called "terms"
  "terms.result<>",   // the <> syntax specifies that we want to bind
                      // to all the slices at once
  "self.result" 
]);

// Create the node to hold the result, add termsNode as a
// dependency and append the binding for sumTermsOp

var sumNode = fabricClient.DG.createNode("sumNode");
sumNode.addMember("result", "Scalar");
sumNode.setDependency(termsNode, "terms");
sumNode.bindings.append(sumTermsBinding);

// Evaluate the sumNode (which evalutes its dependecy, the
// termsNode, first) and then print the result.

sumNode.evaluate();
console.log(sumNode.getData('result', 0));

// Close the Fabric Engine client.  If the client isn't closed
// then Node.js will keep this script alive!

fabricClient.close();
