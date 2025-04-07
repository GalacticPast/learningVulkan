obj_dir := obj
src_dir := app
bin_dir := bin
cc := clang++

ifeq ($(OS),Windows_NT)
vulkan_sdk := $(shell echo %VULKAN_SDK%)

assembly := learningVulkan
extension := .exe
defines := -DDEBUG -DDPLATFORM_WINDOWS
includes := -Isrc -I$(vulkan_sdk)\Include
linker_flags := -luser32 -lvulkan-1 -L$(vulkan_sdk)\Lib 
compiler_flags := -Wall -Wextra -g -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap
build_platform := windows

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
src_files := $(call rwildcard,$(src_dir)/,*.cpp)
directories := $(shell dir src /b /a:d)
obj_files := $(patsubst %.c, $(obj_dir)/%.o, $(src_files))

else

is_linux := $(shell uname -s)

ifeq ($(is_linux),Linux)


assembly := learningVulkan
extension := 
includes := -Iapp/src -I$(VULKAN_SDK)/include
compiler_flags := -Wall -Wextra -g -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap
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
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(obj_dir)\$(src_dir)\,$(subst D:\projects\learningVulkan\src\,,$(directories))) 2>NUL || cd .
else
	@mkdir -p $(bin_dir)
	@mkdir -p $(obj_dir)
	@mkdir -p $(dir $(obj_files_c))
	@mkdir -p $(dir $(obj_files_cpp))
endif

$(obj_dir)/%.cpp.o : %.cpp
	@echo $<...
	@$(cc) $< $(compiler_flags) -c  -o $@ $(defines) $(includes) 

$(obj_dir)/%.c.o : %.c
	@echo $<...
	@clang $< $(compiler_flags) -c -o $@ $(defines) $(includes)


link: $(obj_files_c) $(obj_files_cpp)
	@echo Linking 
	@$(cc) $(compiler_flags) $^ -o $(bin_dir)/$(assembly)$(extension) $(includes) $(defines) $(linker_flags) 
