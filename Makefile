# Makefile for AMX Mod X MemStore
MAKEFLAGS += --warn-undefined-variables --silent

BUILD_DIR = build_linux
BUILD_NAME = memstore_amxx_i386.so

SRCDIR = src
SDKDIR = sdk

SOURCES = $(wildcard $(SRCDIR)/*.cpp) $(SDKDIR)/amxxmodule.cpp
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -fcommon -m32 -O3 -fPIC \
           -funroll-loops -fomit-frame-pointer -fvisibility=hidden -fvisibility-inlines-hidden \
           -DNDEBUG -Dlinux -D__linux__ -DHAVE_STDINT_H -D_GLIBCXX_USE_CXX11_ABI=0

INCLUDES = -I$(SRCDIR) -I$(SDKDIR)
LDFLAGS = -m32 -shared -s -static-libgcc -static-libstdc++

all: prebuild $(BUILD_DIR)/$(BUILD_NAME) postbuild

prebuild:
	mkdir -p $(BUILD_DIR)/$(SRCDIR) $(BUILD_DIR)/$(SDKDIR)

$(BUILD_DIR)/$(BUILD_NAME): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

postbuild:
	mkdir -p package/addons/amxmodx/modules package/addons/amxmodx/scripting/include
	cp $(BUILD_DIR)/$(BUILD_NAME) package/addons/amxmodx/modules/
	cp include/memstore.inc package/addons/amxmodx/scripting/include/
	cp plugins/test_memstore.sma package/addons/amxmodx/scripting/
	cp plugins/example_top15.sma package/addons/amxmodx/scripting/

clean:
	rm -rf $(BUILD_DIR) package
