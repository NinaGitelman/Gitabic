########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings
CXX = g++-10
CXXFLAGS = -std=c++20 -Wall -g
LDFLAGS = -lrt -pthread 

NICE_CFLAGS = $(shell pkg-config --cflags nice)
NICE_LDFLAGS = $(shell pkg-config --libs nice)
NICE_LIBS = -lnice -lgio-2.0 -lgobject-2.0 -lglib-2.0

# Project structure
APPNAME = peer.out
CPP_EXT = .cpp
CPP_SRCDIR = src
OBJDIR = obj

# Source files
CPP_SRC = $(shell find $(CPP_SRCDIR) -type f -name *$(CPP_EXT))

# Object files
CPP_OBJ = $(CPP_SRC:$(CPP_SRCDIR)/%$(CPP_EXT)=$(OBJDIR)/cpp/%.o)
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


# Create dependency files for .cpp
$(OBJDIR)/cpp/%.d: $(CPP_SRCDIR)/%$(CPP_EXT)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(NICE_CFLAGS) -MM $< -MT $(@:%.d=%.o) > $@

# Include dependencies
-include $(DEP)

# Clean build artifacts
.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(APPNAME)
