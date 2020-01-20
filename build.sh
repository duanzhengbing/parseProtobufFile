#!/bin/bash

if [ ! -d "./build" ]; then
	mkdir ./build
fi

cd ./build
rm -rf *

cmake ../
make

cd ../
adb push ./build/parse_template /data/protobuf
adb push ./template/template_file.data /data/protobuf
