JULIAHOME = $(abspath ../julia)
include $(JULIAHOME)/Make.inc

override JULIA_COMMIT = $(shell cd $(JULIAHOME);git rev-parse --short=10 HEAD)
override CFLAGS += $(JCFLAGS)
override CXXFLAGS += $(JCXXFLAGS)

#override Make.inc
ifeq ($(OS), Linux)
override OSLIBS = -ldl -lrt -lpthread -Wl,--export-dynamic -Wl,--version-script=julia-api.expmap -Wl,--no-whole-archive $(LIBUNWIND)
endif

ifeq ($(OS), FreeBSD)
override OSLIBS = -lkvm -lrt -Wl,--export-dynamic -Wl,--version-script=julia-api.expmap $(NO_WHOLE_ARCHIVE) $(LIBUNWIND)
endif
 

ifeq ($(OS), WINNT)
override OSLIBS = -Wl,--export-all-symbols -Wl,--version-script=julia-api.expmap $(NO_WHOLE_ARCHIVE) -lpsapi -lkernel32 -lws2_32 -liphlpapi -lwinmm
ifeq ($(ARCH),x86_64)
override OSLIBS += -ldbghelp
endif
endif

SRCS = \
	jltypes gf ast builtins module codegen interpreter \
	alloc dlload sys init task array dump toplevel jl_uv profile

FLAGS = \
	-D_GNU_SOURCE \
	-Wall -Wno-strict-aliasing -fno-omit-frame-pointer \
	-I$(JULIAHOME)/src/flisp -I$(JULIAHOME)/src/support -fvisibility=hidden -fno-common \
	-I$(call exec,$(LLVM_CONFIG) --includedir) \
	-I$(LIBUV_INC) -I$(JULIAHOME)/usr/include -I$(JULIAHOME)/src


LIBS = $(WHOLE_ARCHIVE) $(JULIAHOME)/src/flisp/libflisp.a $(WHOLE_ARCHIVE) $(JULIAHOME)/src/support/libsupport.a -L$(BUILD)/lib $(LIBUV) $(NO_WHOLE_ARCHIVE) $(call exec,$(LLVM_CONFIG) --libs) $(call exec,$(LLVM_CONFIG) --ldflags) $(OSLIBS)

ifneq ($(MAKECMDGOALS),debug)
TARGET =
else
TARGET = "debug"
endif

OBJS = $(SRCS:%=$(JULIAHOME)/src/%.o) julia-api.o
DOBJS = $(SRCS:%=$(JULIAHOME)/src/%.do) julia-api.do
DEBUGFLAGS += $(FLAGS)
SHIPFLAGS += $(FLAGS)

ifeq ($(JULIAGC),MARKSWEEP)
SRCS += gc
endif

ifeq ($(USE_COPY_STACKS),1)
JCFLAGS += -DCOPY_STACKS
endif

default: api

api : %: libjulia-%

HEADERS = $(JULIAHOME)/src/julia.h julia-api.h $(wildcard support/*.h) $(LIBUV_INC)/uv.h

%.o: %.c $(HEADERS)
	@$(PRINT_CC) $(CC) $(CPPFLAGS) $(CFLAGS) $(SHIPFLAGS) -DNDEBUG -c $< -o $@
%.do: %.c $(HEADERS)
	@$(PRINT_CC) $(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) -c $< -o $@
%.o: %.cpp $(HEADERS)
	@$(PRINT_CC) $(CXX) $(call exec,$(LLVM_CONFIG) --cxxflags) $(CPPFLAGS) $(CXXFLAGS) $(SHIPFLAGS) -c $< -o $@
%.do: %.cpp $(HEADERS)
	@$(PRINT_CC) $(CXX) $(call exec,$(LLVM_CONFIG) --cxxflags) $(CPPFLAGS) $(CXXFLAGS) $(DEBUGFLAGS) -c $< -o $@

libjulia-api.$(SHLIB_EXT): cppAddOn.o julia-api.o
	@$(PRINT_LINK) $(CXX) $(SHIPFLAGS) $(OBJS) cppAddOn.o $(RPATH_ORIGIN) -shared -o $@ $(LDFLAGS) $(LIBS) $(SONAME)
	$(INSTALL_NAME_CMD)libjulia-api.$(SHLIB_EXT) $@
libjulia-api.a: julia.expmap $(OBJS) flisp/libflisp.a support/libsupport.a
	rm -f $@
	@$(PRINT_LINK) ar -rcs $@ $(OBJS)
libjulia-api: libjulia-api.$(SHLIB_EXT)

install: api
	cp libjulia-api.$(SHLIB_EXT) $(JULIAHOME)/julia-$(JULIA_COMMIT)/$(JL_PRIVATE_LIBDIR)
	cp julia-api.h $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia
ifeq ($(OS), Linux)
	#cp $(JULIAHOME)/src/support/htableh.inc $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia
	cp  $(JULIAHOME)/usr/include/libunwind* $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia
endif
ifeq ($(OS), WINNT)
	cp $(JULIAHOME)/usr/include/tree.h $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia
endif

win-dist:
	mkdir -p dist/$(JL_PRIVATE_LIBDIR)
	mkdir -p dist/include/julia
	cp libjulia-api.$(SHLIB_EXT) dist/$(JL_PRIVATE_LIBDIR)
	cp julia-api.h dist/include/julia
	cp $(JULIAHOME)/usr/include/tree.h dist/include/julia
	cd dist && zip -r ../julia-api-$(JULIA_COMMIT)-$(OS)-$(ARCH).zip .
	rm -fr dist

link-jlapi:
	-ln -s $(JULIAHOME)/julia-$(JULIA_COMMIT) $(HOME)/.jlapi/julia-$(JULIA_COMMIT)
	-rm  $(HOME)/.jlapi/julia
	-ln -s $(HOME)/.jlapi/julia-$(JULIA_COMMIT) $(HOME)/.jlapi/julia

clean:
	-rm -f libjulia*
	-rm -f *.do *.o *~ *# *.$(SHLIB_EXT) *.a 
	-rm $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia/julia-api.h
	-rm $(JULIAHOME)/julia-$(JULIA_COMMIT)/$(JL_PRIVATE_LIBDIR)/libjulia-api.$(SHLIB_EXT)
ifeq ($(OS), Linux)
	-rm $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia/htableh.inc
	-rm $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia/libunwind*
endif
ifeq ($(OS), WINNT)
	-rm $(JULIAHOME)/julia-$(JULIA_COMMIT)/include/julia/tree.h
endif
