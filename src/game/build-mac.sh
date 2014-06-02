#!/bin/sh

CXX=g++-4.9
#CXX=clang++

$CXX -O2 -s -std=c++0x -fopenmp -DNO_OPENSSL_SHA sha512.cpp ../moto-engine.cpp ../moto-protocol.cpp game.cpp graphics.cpp render.cpp \
    -I../ -I/usr/include/ \
    -framework OpenGL \
    -lGLEW -lglfw3 -lpthread \
    -o motogame
