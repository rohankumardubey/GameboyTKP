#!/bin/sh
../../build/TKPEmu -p -r -T "./" -s
>./TEST_RESULTS.md
echo "| Expected result | Actual result |" >> ./TEST_RESULTS.md
echo "| --- | --- |" >> ./TEST_RESULTS.md
for f in $(find ./ -name '*.png'); do
    extension="${f##*.}"
    filename="${f%.*}"
    echo "| ![]($f) | ![](${filename}_result.bmp) |" >> ./TEST_RESULTS.md
done