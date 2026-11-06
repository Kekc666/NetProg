CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -O2
TARGET   := daytime_client
SOURCES  := main.cpp udp_client.cpp
OBJECTS  := $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp udp_client.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)
