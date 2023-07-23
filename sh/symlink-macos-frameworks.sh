#!/bin/sh
SYMLINKS_DIR=~/Documents/MacOSX.sdk-Intellisense/

if [ ! -d $SYMLINKS_DIR ]; then
	echo "Adding symlinks for macOS system frameworks to $SYMLINKS_DIR"

	mkdir $SYMLINKS_DIR

	FRAMEWORKS_DIR="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/"

	for dir in $FRAMEWORKS_DIR/*/; do
		if [ -d "$dir/Headers" ]; then
			FOLDER_NAME="$(basename "$dir")"
			ln -s "$dir/Headers" $SYMLINKS_DIR"${FOLDER_NAME%.*}"
		fi
	done
fi