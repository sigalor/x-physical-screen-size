PROJECT_NAME = x-physical-screen-size

CFLAGS = -s -std=c++11 -Wall -O2
LDFLAGS = -s -Wall -O2 -lX11 -lXrandr



all: bin/$(PROJECT_NAME)

bin/$(PROJECT_NAME): obj/$(PROJECT_NAME).o
	mkdir -p bin
	g++ -o $@ $? $(LDFLAGS)

obj/$(PROJECT_NAME).o: src/main.cpp
	mkdir -p obj
	g++ -c -o $@ $? $(CFLAGS)

clean:
	rm -rf bin/$(PROJECT_NAME) obj/$(PROJECT_NAME).o



.PHONY: all clean
