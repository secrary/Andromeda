
CXX:=clang++

CFLAGS:=-g -O0 -Ilibs -Islicer/export  
LDFLAGS:=-lz -lcrypto -std=c++1z 
FILES=Andromeda/Andromeda.cpp slicer/*.cc libs/AxmlParser/AxmlParser.c libs/pugixml/pugixml.cpp libs/miniz/miniz.c libs/disassambler/dissasembler.cc 

detected_OS := $(shell uname)

ifeq ($(detected_OS),Darwin)
    CFLAGS += -I/usr/local/opt/openssl@1.1/include
    LDFLAGS += -L/usr/local/opt/openssl@1.1/lib
endif

ifeq ($(detected_OS),Linux)
    LDFLAGS += -lstdc++fs
endif

bin/andromeda: bin 
	${CXX} ${CFLAGS} ${FILES} ${LDFLAGS} -o bin/andromeda 

bin: 
	mkdir bin

clean:
	rm -rf bin/*
