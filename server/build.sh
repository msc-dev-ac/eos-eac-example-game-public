#!/bin/sh
rm -rf build
mkdir build/
cd build
cmake ..
make -j4
cp MyEOSGameServer ../dist
cd ../dist
GAME_DIR=$(pwd)
cd ../SDK/Tools/devtools/
pwd
./anticheat_integritytool -target_game_dir $GAME_DIR -productid PRODUCT_ID_HERE
cd $GAME_DIR
cd ..