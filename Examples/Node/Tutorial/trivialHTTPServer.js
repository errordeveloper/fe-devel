var fabric = require('Fabric');
var http = require('http');

http.createServer(function (req, res) {
  (function (fabricClient) {
    res.writeHead(200, {'Content-Type': 'text/plain'});
    res.end(fabricClient.build.getName() + " version " + fabricClient.build.getFullVersion() + "\n");
    fabricClient.close();
  })(fabric.createClient());
}).listen(1337, "127.0.0.1");

console.log('Server running at http://127.0.0.1:1337/');
