CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET   = sgbd_hash
SRC_DIR  = src
SRCS     = $(SRC_DIR)/main.cpp $(SRC_DIR)/hash_index.cpp $(SRC_DIR)/operacoes.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/hash_index.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

cleanall: clean
	rm -f out.txt
	rm -rf index/

.PHONY: all clean cleanall