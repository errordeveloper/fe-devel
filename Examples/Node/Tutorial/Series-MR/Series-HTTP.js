var fabricClient = require('Fabric').createClient();
var fs = require('fs');
var http = require('http');
var url = require('url');

// Load the source code and compile into an executable

var compilation = fabricClient.KLC.createCompilation();
compilation.addSource('computeTerm.kl', fs.readFileSync('computeTerm.kl', 'utf8'));
compilation.addSource('subTerms.kl', fs.readFileSync('sumTerms.kl', 'utf8'));
var executable = compilation.run();

// Create the HTTP server
http.createServer(function (req, res) {

  // Take the number of terms to compute from the 'n' GET
  // parameter; default to 10
  var numTerms = parseInt(url.parse(req.url, true).query['n']) || 10;

  // Compute the result using a simple map-reduce
  var result = fabricClient.MR.createReduce(
    fabricClient.MR.createArrayGenerator(
      fabricClient.MR.createConstValue('Size', numTerms),
      executable.resolveArrayGeneratorOperator('computeTerm')
      ),
    executable.resolveReduceOperator('sumTerms')
    ).produce();

  // Return the result as the HTTP content

  res.writeHead(200, {'Content-Type': 'text/plain'});
  res.end(result + "\n");

}).listen(1337, "127.0.0.1");

console.log('Server running at http://127.0.0.1:1337/');

