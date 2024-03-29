CXX := g++
CXXFLAGS := -Wall -std=c++20 -Iinclude -g
SRCDIR := src
OBJDIR := obj
INCDIR := include
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
TARGET := ipk24chat-client

.PHONY: build clean directories

build: directories $(TARGET)

directories:
	mkdir -p $(OBJDIR) $(INCDIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)
