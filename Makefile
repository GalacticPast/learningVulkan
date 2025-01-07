
obj_dir:= obj
bin_dir:= bin 
cc:= clang

is_linux:= $(shell uname -s)

ifeq ($(is_linux),Linux)

linux_platform:= $(shell echo "$$XDG_SESSION_TYPE")

ifeq ($(linux_platform),wayland)		
assembly:= learningVulkan
extension:= 
defines:= -D_DEBUG -DPLATFORM_LINUX_WAYLAND
includes:= -Isrc -lvulkan -lwayland-client 
compiler_flags:= -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap
else ifeq ($(linux_platform),x11)		
assembly:= learningVulkan
extension:= 
defines:= -D_DEBUG -DPLATFORM_LINUX_X11 
includes:= -Isrc -lvulkan -lX11 -lxcb -lX11-xcb -L/usr/X11R6/lib
compiler_flags= -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap
endif

src_files := $(shell find -type f -name '*.c')
obj_files :=  

endif

test: 
	@echo $(src_files)
	@echo $(obj_files)

scaffold:
	echo "scaffolding project structure"
	mkdir -p obj 
		
