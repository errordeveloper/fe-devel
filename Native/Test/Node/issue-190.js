/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var desc = {
  members: { x: undefined }
};
FABRIC.RT.registerType( 'Foobar', desc );
FABRIC.flush();
FABRiC.close();

