#!bin/bash
echo "Building and linking SimpleArkanoid"
clang++ SimpleArkanoid.cpp -o build/SimpleArkanoid -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.11 \
         -Wl,-rpath,. -L./lib/ -lsfml-window -lsfml-graphics -lsfml-system
cd lib
echo "Copying libs"
cp -r ./ ../build
cd ..
echo "Ready"