# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/lily/HAPPY/LAB/apiusage/artifact/SVF

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lily/HAPPY/LAB/apiusage/artifact/SVF

# Include any dependencies generated for this target.
include svf-llvm/tools/CFL/CMakeFiles/cfl.dir/depend.make

# Include the progress variables for this target.
include svf-llvm/tools/CFL/CMakeFiles/cfl.dir/progress.make

# Include the compile flags for this target's objects.
include svf-llvm/tools/CFL/CMakeFiles/cfl.dir/flags.make

svf-llvm/tools/CFL/CMakeFiles/cfl.dir/cfl.cpp.o: svf-llvm/tools/CFL/CMakeFiles/cfl.dir/flags.make
svf-llvm/tools/CFL/CMakeFiles/cfl.dir/cfl.cpp.o: svf-llvm/tools/CFL/cfl.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lily/HAPPY/LAB/apiusage/artifact/SVF/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object svf-llvm/tools/CFL/CMakeFiles/cfl.dir/cfl.cpp.o"
	cd /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/cfl.dir/cfl.cpp.o -c /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL/cfl.cpp

svf-llvm/tools/CFL/CMakeFiles/cfl.dir/cfl.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cfl.dir/cfl.cpp.i"
	cd /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL/cfl.cpp > CMakeFiles/cfl.dir/cfl.cpp.i

svf-llvm/tools/CFL/CMakeFiles/cfl.dir/cfl.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cfl.dir/cfl.cpp.s"
	cd /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL/cfl.cpp -o CMakeFiles/cfl.dir/cfl.cpp.s

# Object files for target cfl
cfl_OBJECTS = \
"CMakeFiles/cfl.dir/cfl.cpp.o"

# External object files for target cfl
cfl_EXTERNAL_OBJECTS =

bin/cfl: svf-llvm/tools/CFL/CMakeFiles/cfl.dir/cfl.cpp.o
bin/cfl: svf-llvm/tools/CFL/CMakeFiles/cfl.dir/build.make
bin/cfl: svf-llvm/libSvfLLVM.a
bin/cfl: /usr/local/lib/libLLVMAnalysis.a
bin/cfl: /usr/local/lib/libLLVMBitWriter.a
bin/cfl: /usr/local/lib/libLLVMCore.a
bin/cfl: /usr/local/lib/libLLVMInstCombine.a
bin/cfl: /usr/local/lib/libLLVMInstrumentation.a
bin/cfl: /usr/local/lib/libLLVMipo.a
bin/cfl: /usr/local/lib/libLLVMIRReader.a
bin/cfl: /usr/local/lib/libLLVMLinker.a
bin/cfl: /usr/local/lib/libLLVMScalarOpts.a
bin/cfl: /usr/local/lib/libLLVMSupport.a
bin/cfl: /usr/local/lib/libLLVMTarget.a
bin/cfl: /usr/local/lib/libLLVMTransformUtils.a
bin/cfl: /usr/lib/x86_64-linux-gnu/libz3.so
bin/cfl: /usr/local/lib/libLLVMBitWriter.a
bin/cfl: /usr/local/lib/libLLVMInstrumentation.a
bin/cfl: /usr/local/lib/libLLVMAsmParser.a
bin/cfl: /usr/local/lib/libLLVMInstCombine.a
bin/cfl: /usr/local/lib/libLLVMAggressiveInstCombine.a
bin/cfl: /usr/local/lib/libLLVMFrontendOpenMP.a
bin/cfl: /usr/local/lib/libLLVMVectorize.a
bin/cfl: /usr/local/lib/libLLVMTransformUtils.a
bin/cfl: /usr/local/lib/libLLVMAnalysis.a
bin/cfl: /usr/local/lib/libLLVMObject.a
bin/cfl: /usr/local/lib/libLLVMBitReader.a
bin/cfl: /usr/local/lib/libLLVMMCParser.a
bin/cfl: /usr/local/lib/libLLVMTextAPI.a
bin/cfl: /usr/local/lib/libLLVMProfileData.a
bin/cfl: /usr/local/lib/libLLVMCore.a
bin/cfl: /usr/local/lib/libLLVMRemarks.a
bin/cfl: /usr/local/lib/libLLVMBitstreamReader.a
bin/cfl: /usr/local/lib/libLLVMMC.a
bin/cfl: /usr/local/lib/libLLVMBinaryFormat.a
bin/cfl: /usr/local/lib/libLLVMDebugInfoCodeView.a
bin/cfl: /usr/local/lib/libLLVMSupport.a
bin/cfl: /usr/local/lib/libLLVMDemangle.a
bin/cfl: svf-llvm/tools/CFL/CMakeFiles/cfl.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lily/HAPPY/LAB/apiusage/artifact/SVF/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../../bin/cfl"
	cd /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cfl.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
svf-llvm/tools/CFL/CMakeFiles/cfl.dir/build: bin/cfl

.PHONY : svf-llvm/tools/CFL/CMakeFiles/cfl.dir/build

svf-llvm/tools/CFL/CMakeFiles/cfl.dir/clean:
	cd /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL && $(CMAKE_COMMAND) -P CMakeFiles/cfl.dir/cmake_clean.cmake
.PHONY : svf-llvm/tools/CFL/CMakeFiles/cfl.dir/clean

svf-llvm/tools/CFL/CMakeFiles/cfl.dir/depend:
	cd /home/lily/HAPPY/LAB/apiusage/artifact/SVF && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lily/HAPPY/LAB/apiusage/artifact/SVF /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL /home/lily/HAPPY/LAB/apiusage/artifact/SVF /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL /home/lily/HAPPY/LAB/apiusage/artifact/SVF/svf-llvm/tools/CFL/CMakeFiles/cfl.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : svf-llvm/tools/CFL/CMakeFiles/cfl.dir/depend

