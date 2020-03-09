CXX:=clang++
CFLAGS:=-g -O0 -Wall -I/usr/local/opt/openssl@1.1/include -Ilibs -Islicer/export -L/usr/local/opt/openssl@1.1/lib -lz -lcrypto -std=c++17 
FILES=Andromeda/Andromeda.cpp slicer/*.cc libs/AxmlParser/AxmlParser.c libs/pugixml/pugixml.cpp libs/miniz/miniz.c libs/disassambler/dissasembler.cc 

bin/andromeda: bin 
	${CXX} ${CFLAGS} ${FILES} -o bin/andromeda 

bin: 
	mkdir bin
