################################################################################
# Dragoon - Copyright (C) 2010 - Michael Levin
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
################################################################################

# Configuration
-include Makefile.config

# Paths
BUILD := build
SOURCE := src
DOC := doc
PACKAGE := dragoon
PACKAGE_STRING := Dragoon 3.0.0
CONFIG_H_IN := $(SOURCE)/config.h.in
CONFIG_H := $(BUILD)/config.h
DOXYFILE := $(BUILD)/Doxyfile

# Compilation flags
ifeq ($(CHECKED),1)
	CFLAGS += -g -Wall
endif
ifeq ($(GCH),1)
	CONFIG_H_GCH := $(CONFIG_H).gch
else
	CONFIG_H_GCH := $(CONFIG_H)
endif
CFLAGS += -include $(CONFIG_H) $(shell sdl-config --cflags)
LDFLAGS += $(shell sdl-config --libs) -lm -lGL -lGLU -lpng -lSDL_ttf

# Make list of source files
SOURCES := $(shell find $(SOURCE) -name \*.cc)
OBJECTS := $(patsubst $(SOURCE)/%.cc,$(BUILD)/%.o,$(SOURCES))
DEPS := $(patsubst %.o,%.d,$(OBJECTS))

# Phony targets
all: $(PACKAGE)
clean:
	rm -rf $(PACKAGE) $(BUILD) $(DOC) $(DOXYFILE)
distclean: clean
realclean: clean
vars:
	@echo "GCH = $(GCH)"
	@echo "CONFIG_H_GCH = $(CONFIG_H_GCH)"
	@echo "SOURCES = $(SOURCES)"
	@echo "OBJECTS = $(OBJECTS)"
	@echo "DEPS = $(DEPS)"
.PHONY: all clean distclean realclean vars

# Make a dummy config file if we don't have one
Makefile.config:
	echo "# Precompiled header (GCC 4+ only)" > Makefile.config
	echo "GCH := 1" >> Makefile.config
	echo "" >> Makefile.config
	echo "# Debug mode" >> Makefile.config
	echo "CHECKED := 1" >> Makefile.config
	echo "" >> Makefile.config
	echo "# Installation prefix" >> Makefile.config
	echo "PREFIX := ." >> Makefile.config

# Configuration header
$(CONFIG_H): Makefile Makefile.config $(CONFIG_H_IN)
	@mkdir -p $(dir $@)
	cat $(CONFIG_H_IN) > $(CONFIG_H)
	echo "" >> $(CONFIG_H)
	echo "// Configured parameters" >> $(CONFIG_H)
	echo "#define PACKAGE \"$(PACKAGE)\"" >> $(CONFIG_H)
	echo "#define PACKAGE_STRING \"$(PACKAGE_STRING)\"" >> $(CONFIG_H)
	echo "#define PKGDATADIR \"$(PREFIX)/dragoon\"" >> $(CONFIG_H)
	echo "#define CHECKED $(CHECKED)" >> $(CONFIG_H)
	echo "#define WINDOWS 0" >> $(CONFIG_H)

# Automatically generate Doxygen config
$(DOXYFILE): Makefile Makefile.config
	@mkdir -p $(dir $@)
	echo "PROJECT_NAME = $(PACKAGE_STRING)" > $(DOXYFILE)
	echo "OUTPUT_DIRECTORY = $(DOC)" >> $(DOXYFILE)
	echo "JAVADOC_AUTOBRIEF = YES" >> $(DOXYFILE)
	echo "QUIET = YES" >> $(DOXYFILE)
	echo "INPUT = $(SOURCE)" >> $(DOXYFILE)
	echo "RECURSIVE = YES" >> $(DOXYFILE)
	echo "HTML_OUTPUT = ." >> $(DOXYFILE)
	echo "GENERATE_LATEX = NO" >> $(DOXYFILE)
	echo "SEARCHENGINE = NO" >> $(DOXYFILE)


# Documentation using Doxygen
$(DOC): $(DOXYFILE)
	doxygen $<

# Precompiled header
$(CONFIG_H).gch: $(CONFIG_H)
	rm -f $@
	$(CXX) -x c++-header -c $(CFLAGS) $(CPPFLAGS) $< -o $@

# Compile objects
$(BUILD)/%.o: $(SOURCE)/%.cc $(CONFIG_H_GCH)
	@mkdir -p $(dir $@)
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
	$(CXX) -MT $@ -MM $(CFLAGS) $(CPPFLAGS) $< -o $(patsubst %.o,%.d,$@)

# Link program
$(PACKAGE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

# Include dependency files
-include $(DEPS)
