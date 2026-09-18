CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic -static -static-libgcc -static-libstdc++
LDLIBS ?= -lgdi32 -luser32

all: pocketcpp.exe

pocketcpp.exe: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp $(LDLIBS) -o pocketcpp.exe

clean:
	$(RM) pocketcpp.exe
