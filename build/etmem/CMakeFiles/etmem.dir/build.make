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
CMAKE_SOURCE_DIR = /home/new/etmem

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/new/etmem/build

# Include any dependencies generated for this target.
include etmem/CMakeFiles/etmem.dir/depend.make

# Include the progress variables for this target.
include etmem/CMakeFiles/etmem.dir/progress.make

# Include the compile flags for this target's objects.
include etmem/CMakeFiles/etmem.dir/flags.make

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem.c.o: etmem/CMakeFiles/etmem.dir/flags.make
etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem.c.o: ../etmem/src/etmem_src/etmem.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem.c.o"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/etmem.dir/src/etmem_src/etmem.c.o   -c /home/new/etmem/etmem/src/etmem_src/etmem.c

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/etmem.dir/src/etmem_src/etmem.c.i"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/new/etmem/etmem/src/etmem_src/etmem.c > CMakeFiles/etmem.dir/src/etmem_src/etmem.c.i

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/etmem.dir/src/etmem_src/etmem.c.s"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/new/etmem/etmem/src/etmem_src/etmem.c -o CMakeFiles/etmem.dir/src/etmem_src/etmem.c.s

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.o: etmem/CMakeFiles/etmem.dir/flags.make
etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.o: ../etmem/src/etmem_src/etmem_project.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.o"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.o   -c /home/new/etmem/etmem/src/etmem_src/etmem_project.c

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.i"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/new/etmem/etmem/src/etmem_src/etmem_project.c > CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.i

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.s"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/new/etmem/etmem/src/etmem_src/etmem_project.c -o CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.s

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.o: etmem/CMakeFiles/etmem.dir/flags.make
etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.o: ../etmem/src/etmem_src/etmem_obj.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.o"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.o   -c /home/new/etmem/etmem/src/etmem_src/etmem_obj.c

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.i"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/new/etmem/etmem/src/etmem_src/etmem_obj.c > CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.i

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.s"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/new/etmem/etmem/src/etmem_src/etmem_obj.c -o CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.s

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.o: etmem/CMakeFiles/etmem.dir/flags.make
etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.o: ../etmem/src/etmem_src/etmem_engine.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.o"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.o   -c /home/new/etmem/etmem/src/etmem_src/etmem_engine.c

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.i"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/new/etmem/etmem/src/etmem_src/etmem_engine.c > CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.i

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.s"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/new/etmem/etmem/src/etmem_src/etmem_engine.c -o CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.s

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.o: etmem/CMakeFiles/etmem.dir/flags.make
etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.o: ../etmem/src/etmem_src/etmem_rpc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.o"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.o   -c /home/new/etmem/etmem/src/etmem_src/etmem_rpc.c

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.i"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/new/etmem/etmem/src/etmem_src/etmem_rpc.c > CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.i

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.s"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/new/etmem/etmem/src/etmem_src/etmem_rpc.c -o CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.s

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.o: etmem/CMakeFiles/etmem.dir/flags.make
etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.o: ../etmem/src/etmem_src/etmem_common.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.o"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.o   -c /home/new/etmem/etmem/src/etmem_src/etmem_common.c

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.i"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/new/etmem/etmem/src/etmem_src/etmem_common.c > CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.i

etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.s"
	cd /home/new/etmem/build/etmem && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/new/etmem/etmem/src/etmem_src/etmem_common.c -o CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.s

# Object files for target etmem
etmem_OBJECTS = \
"CMakeFiles/etmem.dir/src/etmem_src/etmem.c.o" \
"CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.o" \
"CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.o" \
"CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.o" \
"CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.o" \
"CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.o"

# External object files for target etmem
etmem_EXTERNAL_OBJECTS =

../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem.c.o
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_project.c.o
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_obj.c.o
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_engine.c.o
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_rpc.c.o
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/src/etmem_src/etmem_common.c.o
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/build.make
../etmem/build/bin/etmem: etmem/CMakeFiles/etmem.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/new/etmem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking C executable ../../etmem/build/bin/etmem"
	cd /home/new/etmem/build/etmem && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/etmem.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
etmem/CMakeFiles/etmem.dir/build: ../etmem/build/bin/etmem

.PHONY : etmem/CMakeFiles/etmem.dir/build

etmem/CMakeFiles/etmem.dir/clean:
	cd /home/new/etmem/build/etmem && $(CMAKE_COMMAND) -P CMakeFiles/etmem.dir/cmake_clean.cmake
.PHONY : etmem/CMakeFiles/etmem.dir/clean

etmem/CMakeFiles/etmem.dir/depend:
	cd /home/new/etmem/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/new/etmem /home/new/etmem/etmem /home/new/etmem/build /home/new/etmem/build/etmem /home/new/etmem/build/etmem/CMakeFiles/etmem.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : etmem/CMakeFiles/etmem.dir/depend

