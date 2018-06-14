#!bin/bash
clang++ SimpleArkanoid.cpp -o SimpleArkanoid -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.11 \
         -Wl,-rpath,. -L./lib/ -lsfml-window -lsfml-graphics -lsfml-system
mv SimpleArkanoid ./build/SimpleArkanoid
cd lib
cp -r ./ ../build
cd ..
#ld -o Arkanoid SimpleArkanoid.o ./lib/libsfml-window.2.5.0.dylib ./lib/libsfml-graphics.2.5.0.dylib ./lib/libsfml-system.2.5.0.dylib ./lib/libsfml-audio.2.5.0.dylib ./lib/libsfml-network.2.5.0.dylib