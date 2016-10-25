CFLAGS=$(shell pkg-config --cflags eigen3 sdl2 SDL2_ttf)
LDFLAGS=$(shell pkg-config --libs eigen3 sdl2 SDL2_ttf)
BIN=moja

ifeq ($(OS),Windows_NT)
BIN:=$(BIN).exe
endif

all : $(BIN)

$(BIN) : main.o
	$(CXX) -g -o $@ $< $(LDFLAGS)

main.o : main.cpp bmp.hpp
	$(CXX) -g $(CFLAGS) -c main.cpp

clean :
	-rm *.o $(BIN)
