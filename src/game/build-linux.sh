#!/bin/sh

CXX=g++
#CXX=clang++

$CXX -O2 -s -std=c++0x -fopenmp -DNO_OPENSSL_SHA sha512.cpp ../moto-engine.cpp ../moto-protocol.cpp game.cpp graphics.cpp render.cpp -I../ -I/usr/include/ -lGL -lGLEW -lglfw3 -lX11 -lXxf86vm -lXrandr -lXi -lpthread -o motogame