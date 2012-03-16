var fabricClient = require('Fabric').createClient();
var fs = require('fs');

// Take the number of terms to compute on the command
// line; default to 10

var numTerms = parseInt(process.argv[2]) || 10;

// Load the source code and compile into an executable

var compilation = fabricClient.KLC.createCompilation();
compilation.addSource('computeTerm.kl', fs.readFileSync('computeTerm.kl', 'utf8'));
compilation.addSource('sumTerms.kl', fs.readFileSync('sumTerms.kl', 'utf8'));
var executable = compilation.run();

// Summing a series is a simple map-reduce operation where
// we generate the terms of the series
console.log(
  fabricClient.MR.createReduce(
    fabricClient.MR.createArrayGenerator(
      fabricClient.MR.createConstValue('Size', numTerms),
      executable.resolveArrayGeneratorOperator('computeTerm')
      ),
    executable.resolveReduceOperator('sumTerms')
    ).produce()
  );

// Close the Fabric Engine client.  If the client isn't closed
// then Node.js will keep this script alive!

fabricClient.close();
