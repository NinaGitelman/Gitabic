########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings
CXX = g++-10
CC = gcc
CXXFLAGS = -std=c++20 -Wall -g
CFLAGS = -Wall -g -std=c99
LDFLAGS = -lrt -pthread 

# libnice flags
NICE_CFLAGS = $(shell pkg-config --cflags nice)
NICE_LDFLAGS = $(shell pkg-config --libs nice)
NICE_LIBS = -lnice -lgio-2.0 -lgobject-2.0 -lglib-2.0

# Project structure
APPNAME = peer.out
CPP_EXT = .cpp
C_EXT = .c
CPP_SRCDIR = src/cpp
C_SRCDIR = src/c
OBJDIR = obj

# Source files
CPP_SRC = $(shell find $(CPP_SRCDIR) -type f -name *$(CPP_EXT))
C_SRC = $(shell find $(C_SRCDIR) -type f -name *$(C_EXT))

# Object files
CPP_OBJ = $(CPP_SRC:$(CPP_SRCDIR)/%$(CPP_EXT)=$(OBJDIR)/cpp/%.o)
C_OBJ = $(C_SRC:$(C_SRCDIR)/%$(C_EXT)=$(OBJDIR)/c/%.o)
OBJ = $(CPP_OBJ) $(C_OBJ)

# Dependencies
DEP = $(CPP_OBJ:%.o=%.d) $(C_OBJ:%.o=%.d)

# Clean-up settings
RM = rm -f

########################################################################
####################### Targets beginning here #########################
########################################################################

# Default target
all: $(APPNAME)

# Link object files into the final executable
$(APPNAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(NICE_LDFLAGS)

# Compile .cpp files with libnice include paths
$(OBJDIR)/cpp/%.o: $(CPP_SRCDIR)/%$(CPP_EXT)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(NICE_CFLAGS) -c $< -o $@

# Compile .c files with libnice include paths
$(OBJDIR)/c/%.o: $(C_SRCDIR)/%$(C_EXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(NICE_CFLAGS) -c $< -o $@

# Create dependency files for .cpp
$(OBJDIR)/cpp/%.d: $(CPP_SRCDIR)/%$(CPP_EXT)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(NICE_CFLAGS) -MM $< -MT $(@:%.d=%.o) > $@

# Create dependency files for .c
$(OBJDIR)/c/%.d: $(C_SRCDIR)/%$(C_EXT)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(NICE_CFLAGS) -MM $< -MT $(@:%.d=%.o) > $@

# Include dependencies
-include $(DEP)

# Clean build artifacts
.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(APPNAME)
