CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -O2
TARGET   := echo_client
SOURCES  := main.cpp tcp_client.cpp
OBJECTS  := $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp tcp_client.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)
