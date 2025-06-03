obj_dir := obj
src_dir := app
bin_dir := bin
ccplus := clang++
cc := clang

ifeq ($(OS),Windows_NT)

vulkan_sdk := $(shell echo %VULKAN_SDK%)
DIR := $(subst /,\,${CURDIR})

assembly := learningVulkan
extension := .exe
defines := -DDEBUG -DDPLATFORM_WINDOWS
# Turn this on if you want tracy profiler
#defines += -DTRACY_ENABLE
includes := -Iapp/tests -I$(src_dir)/src -I$(vulkan_sdk)/Include
linker_flags := -lgdi32 -luser32 -lvulkan-1 -L$(vulkan_sdk)/Lib -ladvapi32 -ltdh -lWinmm
compiler_flags := -Wall -Wextra -g -O0 -Wno-system-headers -Wno-unused-but-set-variable -Wno-unused-variable -Wno-varargs -Wno-unused-private-field -Wno-unused-parameter -Wno-unused-function -fsanitize=undefined -fsanitize-trap
build_platform := windows

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
src_files_c := $(call rwildcard,$(src_dir)/,*.c)
src_files_cpp := $(call rwildcard,$(src_dir)/,*.cpp)

# Tracy integration
src_files_cpp := $(filter-out $(src_dir)/src/vendor/tracy/%, $(src_files_cpp))
src_files_c := $(filter-out $(src_dir)/src/vendor/tracy/%, $(src_files_c))
#turn this on if you want tracy
#src_files_cpp += $(src_dir)/src/vendor/tracy/TracyClient.cpp

directories:= $(subst $(DIR),,$(shell dir $(src_dir) /S /AD /B | findstr /i $(src_dir) )) # Get all directories under src.

obj_files_c := $(patsubst %.c, $(obj_dir)/%.c.o, $(src_files_c))
obj_files_cpp := $(patsubst %.cpp, $(obj_dir)/%.cpp.o, $(src_files_cpp))
else

is_linux := $(shell uname -s)

ifeq ($(is_linux),Linux)


assembly := learningVulkan
extension :=
includes := -Iapp/src -I$(VULKAN_SDK)/include
compiler_flags := -Wall -Wextra -g -O0 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined  -fsanitize-trap
defines := -DDEBUG
linker_flags := -lvulkan -lm


linux_platform := $(shell echo "$$XDG_SESSION_TYPE")

ifeq ($(linux_platform),wayland)

defines += -DDPLATFORM_LINUX_WAYLAND
linker_flags += -lwayland-client -lxkbcommon

else ifeq ($(linux_platform),x11)
defines += -DDPLATFORM_LINUX_X11
linker_flags += -lX11 -lxcb -lX11-xcb -L/usr/X11R6/lib

endif

src_files_c := $(shell find $(src_dir) -type f -name '*.c')
src_files_cpp := $(shell find $(src_dir) -type f -name '*.cpp')
dependencies := $(shell find $(src_dir) -type d)
obj_files_cpp := $(patsubst %.cpp, $(obj_dir)/%.cpp.o, $(src_files_cpp))
obj_files_c := $(patsubst %.c, $(obj_dir)/%.c.o, $(src_files_c))

endif
endif

all: scaffold link

scaffold:
ifeq ($(OS),Windows_NT)
	@echo scaffolding project structure
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(obj_dir) 2>NUL || cd .
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(bin_dir) 2>NUL || cd .
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addsuffix \$(src_dir),$(obj_dir)) 2>NUL || cd .
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(obj_dir), $(directories)) 2>NUL || cd .
else
	@mkdir -p $(bin_dir)
	@mkdir -p $(obj_dir)
	@mkdir -p $(dir $(obj_files_c))
	@mkdir -p $(dir $(obj_files_cpp))
endif

$(obj_dir)/%.cpp.o : %.cpp
	@echo $<...
	@$(ccplus) $< $(compiler_flags) -c  -o $@ $(defines) $(includes)

$(obj_dir)/%.c.o : %.c
	@echo $<...
	@$(cc) $< $(compiler_flags) -c -o $@ $(defines) $(includes)


link: $(obj_files_c) $(obj_files_cpp)
	@echo Linking
	@$(ccplus) $(compiler_flags) $^ -o $(bin_dir)/$(assembly)$(extension) $(includes) $(defines) $(linker_flags)
