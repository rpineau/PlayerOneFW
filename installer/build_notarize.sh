#!/bin/bash

PACKAGE_NAME="PlayerOneFW_X2.pkg"
BUNDLE_NAME="org.rti-zone.PlayerOneFWX2"

install_name_tool -change  libPlayerOnePW.1.dylib @executable_path/../Frameworks/libPlayerOnePW.1.0.0.dylib "../build/Release/libPlayerOneFW.dylib"

if [ ! -z "$app_id_signature" ]; then
    codesign -f -s "$app_id_signature" --verbose ../build/Release/libPlayerOneFW.dylib
    codesign -f -s "$app_id_signature" --verbose "../static_libs/macOS/libPlayerOnePW.1.0.0.dylib"
fi

mkdir -p ROOT/tmp/PlayerOneFW_X2/
cp "../filterwheellist PlayerOneFW.txt" ROOT/tmp/PlayerOneFW_X2/
cp "../PlayerOneFW.ui" ROOT/tmp/PlayerOneFW_X2/
cp "../PlayerOneFWSelect.ui" ROOT/tmp/PlayerOneFW_X2/
cp "../build/Release/libPlayerOneFW.dylib" ROOT/tmp/PlayerOneFW_X2/
cp "../static_libs/macOS/libPlayerOnePW.1.0.0.dylib" ROOT/tmp/PlayerOneFW_X2/


if [ ! -z "$installer_signature" ]; then
	# signed package using env variable installer_signature
	pkgbuild --root ROOT --identifier "$BUNDLE_NAME" --sign "$installer_signature" --scripts Scripts --version 1.0 "$PACKAGE_NAME"
	pkgutil --check-signature "./${PACKAGE_NAME}"
	xcrun notarytool submit "$PACKAGE_NAME" --keychain-profile "$AC_PROFILE" --wait
	xcrun stapler staple $PACKAGE_NAME
else
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --scripts Scripts --version 1.0 $PACKAGE_NAME
fi


rm -rf ROOT
