

# Where to find source code to test
SOURCE_DIR = incl

# Where to find test code.
TEST_DIR = .

# Where the builded files will be stored to.
BUILD_DIR = _build


# The pathes to tinybuf-file-dir, to tinybuf-include-dir, to tinybuf-generator-dir and to output-dir
TINYBUF_DIR := ..
TINYBUF_SRC_DIR := $(TINYBUF_DIR)/incl


# Points to the root of Google Test, relative to where this file is.
# Remember to tweak this if you move this file.
GTEST_DIR = googletest


# If you want no cmd-line print outs of the cmds
NO_ECHO := @

# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = tester


SOURCE_INC_DIR += -I$(SOURCE_DIR)
TINYBUF_INC_DIR += -I$(TINYBUF_SRC_DIR)

#$(info SOURCE_INC_DIR=${SOURCE_INC_DIR})
#$(info TINYBUF_INC_DIR=${TINYBUF_INC_DIR})



CFLAGS += -Wall -Werror -Wextra


# Add the CFLAGS to the CXXFLAGS
CXXFLAGS += $(CFLAGS)



SOURCE_FILES = $(notdir $(wildcard $(SOURCE_DIR)/*.c))
SOURCE_OBJECTS += $(addprefix $(BUILD_DIR)/,$(SOURCE_FILES:.c=.o))

TESTS_OBJECTS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(TESTS)))



TINYBUF_FILES = $(notdir $(wildcard $(TINYBUF_SRC_DIR)/*.c))
TINYBUF_OBJECTS += $(addprefix $(BUILD_DIR)/,$(TINYBUF_FILES:.c=.o))
#$(info TINYBUF_SRC_DIR=${TINYBUF_SRC_DIR})
#$(info TINYBUF_FILES=${TINYBUF_FILES})
#$(info TINYBUF_OBJECTS=${TINYBUF_OBJECTS})


RUN_TESTS = $(addprefix run_, $(TESTS))


all : clean $(TESTS)
	

clean :
	$(NO_ECHO)rm -f -r $(BUILD_DIR)
	$(NO_ECHO)mkdir $(BUILD_DIR)


# Target for running all unit tests
run_all : $(RUN_TESTS)

# Make the run_..-targets Phony.
.PHONY: $(RUN_TESTS)


# Target for running one specific unit test
$(RUN_TESTS) :
	@echo 
	@echo ====================================================================
	@echo Run test: $@
	@echo ====================================================================
	$(NO_ECHO)(cd $(BUILD_DIR)/ ;	./$@)
	@echo ====================================================================



$(SOURCE_OBJECTS):
	@echo Compiling $(notdir $@)
	$(NO_ECHO) $(CXX) $(CPPFLAGS) $(TINYBUF_INC_DIR) $(SOURCE_INC_DIR) $(CXXFLAGS) -o $@ -c $(SOURCE_DIR)/$(basename $(notdir $@)).c*

$(TINYBUF_OBJECTS):
	@echo Compiling $(notdir $@)
	$(NO_ECHO) $(CXX) $(CPPFLAGS) $(TINYBUF_INC_DIR) $(SOURCE_INC_DIR) $(CXXFLAGS) -o $@ -c $(TINYBUF_SRC_DIR)/$(basename $(notdir $@)).c*

$(TESTS_OBJECTS): $(TINYBUF_OBJECTS) $(SOURCE_OBJECTS)
	@echo Compiling $(notdir $@)
#	@echo Test object $(basename $@)
	$(NO_ECHO)$(CXX) $(CPPFLAGS) $(TINYBUF_INC_DIR) $(SOURCE_INC_DIR) $(CXXFLAGS) -o $@ -c $(TEST_DIR)/$(basename $(notdir $@)).c*

#https://stackoverflow.com/questions/16262344/pass-a-target-name-to-dependency-list-in-makefile
$(TESTS): % : $(BUILD_DIR)/%.o 
	@echo Linking $(notdir $@)
	$(NO_ECHO)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TINYBUF_INC_DIR) $(SOURCE_INC_DIR) $(BUILD_DIR)/$@.o $(TINYBUF_OBJECTS) $(SOURCE_OBJECTS) -o $(BUILD_DIR)/run_$@



