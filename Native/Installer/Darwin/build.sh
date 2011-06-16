#!/bin/sh

rm -rf dist

echo Creating dist/Fabric\ NPAPI\ Plugin\ \(32\ Bit\)/Fabric\ NPAPI\ Plugin\ \(32\ Bit\).pkg
mkdir -p dist/Fabric\ NPAPI\ Plugin\ \(32\ Bit\)
/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker \
  --doc Fabric\ NPAPI\ Plugin\ \(32\ Bit\).pmdoc/ \
  --out dist/Fabric\ NPAPI\ Plugin\ \(32\ Bit\)/Fabric\ NPAPI\ Plugin\ \(32\ Bit\).pkg

echo "Creating" Fabric\ NPAPI\ Plugin\ \(32\ Bit\).dmg
rm Fabric\ NPAPI\ Plugin\ \(32\ Bit\).dmg
hdiutil create Fabric\ NPAPI\ Plugin\ \(32\ Bit\).dmg \
  -volname Fabric\ NPAPI\ Plugin\ \(32\ Bit\) \
  -fs HFS+ \
  -srcfolder dist/Fabric\ NPAPI\ Plugin\ \(32\ Bit\)

if false; then
  mkdir -p dist/Fabric\ DevServer
  cp -r ../../DevServer/FabricDevServer.app dist/Fabric\ DevServer/Fabric\ DevServer.app
  ln -s /Applications dist/Fabric\ DevServer/Applications
  rm Fabric\ DevServer.dmg
  hdiutil create Fabric\ DevServer.dmg -volname Fabric\ DevServer -fs HFS+ -srcfolder dist/Fabric\ DevServer
fi
