#!/bin/sh
GBPATH=`cat ./.gbtkp_path`
if [ "$#" -eq 0 ]; then
	if [ -f "./.gbtkp_path" ]; then
		cp -r $(pwd)/. "$GBPATH"	
	else
		printf "TKPEmu gb_tkp path not set.\nRun this script with --set-path \"path/to/gb_tkp_folder\" to configure first\n"
	fi
elif [ "$#" -eq 2 ]; then
	if [ $1 == "--set-path" ]; then
		echo "$2" > ./.gbtkp_path
		printf "Path set\n"
	else
		printf "Unknown parameter $1\n"
	fi
else
	printf "Invalid amount of parameters\n"
fi
