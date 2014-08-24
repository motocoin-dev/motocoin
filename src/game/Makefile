
CPP = g++
CXXFLAGS = -O2 -s -std=c++0x -fopenmp 
SOURCES = ../cJumpPointSearch/src/*.cpp sha512.cpp ../moto-engine.cpp ../moto-protocol.cpp game.cpp graphics.cpp render.cpp
LIBS = -lGL -lGLEW -lglfw3 -lX11 -lXxf86vm -lXrandr -lXi -lpthread
INCLUDES = -I../
LDFLAGS = 
GLINCLUDES = 

all:
	$(CPP) $(CXXFLAGS) -DNO_OPENSSL_SHA $(SOURCES) $(INCLUDES) $(GLINCLUDES) $(LDFLAGS) $(LIBS) -o motogame 
