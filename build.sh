#!bin/bash
clang++ SimpleArkanoid.cpp -o build/SimpleArkanoid -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.11 \
         -Wl,-rpath,. -L./lib/ -lsfml-window -lsfml-graphics -lsfml-system
cd lib
cp -r ./ ../build
cd ..
