#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

CXX      = g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

# -------------------------------------------------------
# OpenSSL — uncomment the block for your machine
#
# macOS (homebrew):
# LFLAGS := -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
#
# Arch Linux:
LFLAGS   := -lssl -lcrypto
# -------------------------------------------------------

OUTPUT  := output
SRC     := src
INCLUDE := include
LIB     := lib

ifeq ($(OS),Windows_NT)
	MAIN        := main.exe
	SOURCEDIRS  := $(SRC)
	INCLUDEDIRS := $(INCLUDE)
	LIBDIRS     := $(LIB)
	FIXPATH      = $(subst /,\,$1)
	RM          := del /q /f
	MD          := mkdir
else
	MAIN        := main
	SOURCEDIRS  := $(shell find $(SRC) -type d 2>/dev/null)
	INCLUDEDIRS := $(shell find $(INCLUDE) -type d 2>/dev/null)
	LIBDIRS     := $(shell find $(LIB) -type d 2>/dev/null)
	FIXPATH      = $1
	RM          := rm -f
	MD          := mkdir -p
endif

INCLUDES := $(patsubst %,-I%,$(INCLUDEDIRS:%/=%))
LIBS     := $(patsubst %,-L%,$(LIBDIRS:%/=%))


SOURCES := $(wildcard $(patsubst %,%/*.cpp,$(SOURCEDIRS)))
OBJECTS := $(SOURCES:.cpp=.o)
DEPS    := $(OBJECTS:.o=.d)

OUTPUTMAIN := $(call FIXPATH,$(OUTPUT)/$(MAIN))

all: $(OUTPUT) $(MAIN)
	@echo "done! binary is at $(OUTPUTMAIN)"

$(OUTPUT):
	$(MD) $(OUTPUT)

$(MAIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS) $(LIBS)

-include $(DEPS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -MMD $< -o $@

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))
	$(RM) $(call FIXPATH,$(DEPS))
	@echo "cleaned!"

.PHONY: run
run: all
	./$(OUTPUTMAIN)