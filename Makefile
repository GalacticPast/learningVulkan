obj_dir := obj
src_dir := src 
bin_dir := bin 
cc := clang

is_linux := $(shell uname -s)

ifeq ($(is_linux),Linux)

linux_platform := $(shell echo "$$XDG_SESSION_TYPE")

ifeq ($(linux_platform),wayland)		

assembly := learningVulkan
extension := 
defines := -D_DEBUG -DPLATFORM_LINUX_WAYLAND
includes := -Isrc 
linker_flags := -lvulkan -lwayland-client 
compiler_flags := -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap

else ifeq ($(linux_platform),x11)		
assembly := learningVulkan
extension := 
defines := -D_DEBUG -DPLATFORM_LINUX_X11 
includes := -Isrc 
linker_flags := -lvulkan -lX11 -lxcb -lX11-xcb -L/usr/X11R6/lib
compiler_flags := -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap
endif

src_files := $(shell find $(src_dir) -type f -name '*.c')
dependencies := $(shell find $(src_dir) -type f -name '*.h')
obj_files := $(patsubst src%.c,obj%.o,$(src_files))

endif

all: scaffold 

scaffold :
	@echo scaffolding project structure...
	@mkdir -p $(obj_dir)
	@mkdir -p $(patsubst src%,obj%,$(dir $(obj_files)))
	@echo done.


%.o: %.c $(dependencies)
		@echo compiling $<...	
		$(cc) -c -o $@ $< $(compiler_flags) $(includes) 


link: $(obj_files)
	@echo linking $^..
	$(cc) -o my_program $^ $(compiler_flags)
