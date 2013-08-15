/*
  julia-api.c extension of jlapi.c
  miscellaneous functions for users of libjulia-api.$(SL_EXT), to handle initialization
  and the style of use where julia is not in control most of the time.
*/
#include "platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#if defined(_OS_WINDOWS_) && !defined(_COMPILER_MINGW_)
char * __cdecl dirname(char *);
char * __cdecl basename(char *);
#else
#include <libgen.h>
#endif
#include "julia-api.h"

/************* Tools ********************/
//-| jlapi_init is a small adaptation of jl_init provided in jlapi.c
//-| Main change: 
//-| -> use of DL_LOAD_PATH (slight modification of init_load_path in client.jl)
//-| -> redirection of STDIN, STDOUT, STDERR
//-|    Have to solve a strange behavior: Base.reinit_stdio fail to reload STDIN, STDOUT,STDERR 
//-|    with as a direct consequence all printing failed.

//-|  mode=="rcqls" is for rcqls development, mode="tty" is for initialization of STDOUT, STDERR with C API)
//-|  other value of mode mean standard jlapi.c
DLLEXPORT void jlapi_init(char *julia_home_dir, char* mode) {
  libsupport_init();
  char *image_file = jl_locate_sysimg(julia_home_dir);
  printf("image-file=%s\n",image_file);
  julia_init(image_file);
  jlapi_mode=mode;

  jl_set_const(jl_core_module, jl_symbol("JULIA_HOME"),
               jl_cstr_to_string(julia_home));
  jl_module_export(jl_core_module, jl_symbol("JULIA_HOME"));
  //-| This avoid LD_PRELOAD on linux since shared objects not exported
  //-| Maybe fix this in a better way with options compilation.
  char julia_api_libname[512];
#if defined(_OS_WINDOWS_)
  const char *shlib_ext=".dll";
  const char *sep="\\";
#elif defined(__APPLE__)
  const char *shlib_ext=".dylib";
  const char *sep="/";
#else
  const char *shlib_ext=".so";
  const char *sep="/";
#endif

  snprintf(julia_api_libname, sizeof(julia_api_libname), "%s%s%s%s%s%s",
          julia_home_dir, sep,"julia",sep,"libjulia-api",shlib_ext);
  load_library_permanently(julia_api_libname);
  if(strcmp(mode,"rcqls")<=0) { // cqls, rcqls
    //-| Called first to fix the DL_LOAD_PATH needed to (dl)open library (libpcre for example)
    //-| Replacement of Base.init_load_path()
    //-| Update 01/08/2013: No need to set DL_LOAD_PATH, just push 
    //-| jl_set_global(jl_base_module,jl_symbol("DL_LOAD_PATH"),jl_eval_string("ByteString[join([JULIA_HOME,\"..\",\"lib\",\"julia\"],Base.path_separator)]"));
    jl_eval_string("Base.push!(DL_LOAD_PATH,join([JULIA_HOME,\"..\",\"lib\",\"julia\"],Base.path_separator))");
    //-| DL_LOAD_PATH is a global constant already defined before and then not overloaded by julia
    //-| Only LOAD_PATH would be initialized (needs libpcre because of abspath)!
    jl_eval_string("vers = \"v$(VERSION.major).$(VERSION.minor)\"");
    jl_set_global(jl_base_module,jl_symbol("LOAD_PATH"),jl_eval_string("ByteString[abspath(JULIA_HOME,\"..\",\"local\",\"share\",\"julia\",\"site\",vers),abspath(JULIA_HOME,\"..\",\"share\",\"julia\",\"site\",vers)]")); 
  } else jl_eval_string("Base.init_load_path()");
  if(strcmp(mode,"tty")==0) {
    jl_eval_string("Base.reinit_stdio()");
    jl_set_global(jl_base_module,jl_symbol("STDIN"),jl_eval_string("Base.init_stdio(ccall(:jl_stdin_stream ,Ptr{Void},()),0)"));
    //-| 2 next lines fails even it is if no more necessary
    //-| Update 27/07/13: no more crash but stuck when print.
    jl_set_global(jl_base_module,jl_symbol("STDOUT"),jl_eval_string("Base.init_stdio(ccall(:jl_stdout_stream,Ptr{Void},()),1)"));
    jl_set_global(jl_base_module,jl_symbol("STDERR"),jl_eval_string("Base.init_stdio(ccall(:jl_stderr_stream,Ptr{Void},()),2)"));
  } else if(strcmp(mode,"rcqls")<=0) { //cqls, rcqls
    jl_eval_string("Base.reinit_stdio()");
    //-| STDIN, STDOUT and STDERR not properly loaded
    //-| I prefer redirection of STDOUT and STDERR in IOBuffer (maybe STDIN ???)
      jl_set_global(jl_base_module,jl_symbol("STDIN"),jl_eval_string("Base.init_stdio(ccall(:jl_stdin_stream ,Ptr{Void},()),0)"));
      //jl_set_global(jl_base_module,jl_symbol("STDIN"),jl_eval_string("IOBuffer()"));
      jl_set_global(jl_base_module,jl_symbol("STDOUT"),jl_eval_string("IOBuffer()"));
      jl_set_global(jl_base_module,jl_symbol("STDERR"),jl_eval_string("IOBuffer()"));
  } else jl_eval_string("Base.reinit_stdio()");
  jl_eval_string("Base.fdwatcher_reinit()");
  jl_eval_string("Base.Random.librandom_init()");
  jl_eval_string("Base.check_blas()");
  jl_eval_string("LinAlg.init()");
  jl_eval_string("Sys.init()");
  jl_eval_string("Base.init_sched()");
  jl_eval_string("Base.init_head_sched()");
  jl_eval_string("Base.try_include(abspath(ENV[\"HOME\"],\".juliarc.jl\"))");
  if(strcmp(mode,"rcqls")==0) { 
    jl_eval_string("println(\"Julia initialized!\")");
    jlapi_print_stdout();
  }
}

//-| Get STDOUT, STDERR IOBuffer as string

DLLEXPORT  const char *jlapi_get_stdout() {
  jl_value_t *out;
  
  if(strcmp(jlapi_mode,"rcqls")==0) {
    out=jl_eval_string("seek(STDOUT, 0);jl4rb_out = takebuf_string(STDOUT);truncate(STDOUT, 0);jl4rb_out");
    return jl_bytestring_ptr(out);
  } else return "";
}

DLLEXPORT  const char *jlapi_get_stderr() {
  jl_value_t *out;
  
  if(strcmp(jlapi_mode,"rcqls")==0) {
    out=jl_eval_string("seek(STDERR, 0);jl4rb_out = takebuf_string(STDERR);truncate(STDERR, 0);jl4rb_out");
    return jl_bytestring_ptr(out);
  } else return ""; 
}

//-| Print STDOUT, STDERR IOBuffer
DLLEXPORT void jlapi_print_stdout() {
  const char *outString;
  
  outString=jlapi_get_stdout();
  if(strlen(outString)) printf("%s\n",outString);
}

DLLEXPORT  void jlapi_print_stderr() {
  const char *outString;
   
  outString= jlapi_get_stderr();
  if(strlen(outString)) printf("%s\n",outString);
}

// From src/jlapi.c
#ifdef COPY_STACKS
void jl_switch_stack(jl_task_t *t, jl_jmp_buf *where);
extern jl_jmp_buf * volatile jl_jmp_target;
#endif

DLLEXPORT char *jl_locate_sysimg(char *jlhome)
{
    if (jlhome == NULL) {
        char *julia_path = (char*)malloc(512);
        size_t path_size = 512;
        uv_exepath(julia_path, &path_size);
        julia_home = strdup(dirname(julia_path));
        free(julia_path);
    }
    else {
        julia_home = jlhome;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s%s%s",
             julia_home, PATHSEPSTRING, JL_SYSTEM_IMAGE_PATH);
    return strdup(path);
}


 
DLLEXPORT void *jl_eval_string(char *str)
{
#ifdef COPY_STACKS
    jl_root_task->stackbase = (char*)&str;
    if (jl_setjmp(jl_root_task->base_ctx, 1)) {
        jl_switch_stack(jl_current_task, jl_jmp_target);
    }
#endif
    jl_value_t *r;
    JL_TRY {
        jl_value_t *ast = jl_parse_input_line(str);
        JL_GC_PUSH1(&ast);
        r = jl_toplevel_eval(ast);
        JL_GC_POP();
    }
    JL_CATCH {
        //jl_show(jl_stderr_obj(), jl_exception_in_transit);
        r = NULL;
    }
    return r;
}

DLLEXPORT jl_value_t *jl_exception_occurred(void)
{
    return jl_is_null(jl_exception_in_transit) ? NULL : 
        jl_exception_in_transit;
}

DLLEXPORT void jl_exception_clear(void)
{
    jl_exception_in_transit = (jl_value_t*)jl_null;
}

// get the name of a type as a string
DLLEXPORT const char *jl_typename_str(jl_value_t *v)
{
    if (jl_is_tuple(v))
        return "Tuple";
    return ((jl_datatype_t*)v)->name->name->name;
}

// get the name of typeof(v) as a string
DLLEXPORT const char *jl_typeof_str(jl_value_t *v)
{
    return jl_typename_str((jl_value_t*)jl_typeof(v));
}

DLLEXPORT void *jl_array_eltype(jl_value_t *a)
{
    return jl_tparam0(jl_typeof(a));
}

DLLEXPORT int jl_array_rank(jl_value_t *a)
{
    return jl_array_ndims(a);
}

DLLEXPORT size_t jl_array_size(jl_value_t *a, int d)
{
    return jl_array_dim(a, d);
}

DLLEXPORT void *jl_array_ptr(jl_array_t *a);

DLLEXPORT const char *jl_bytestring_ptr(jl_value_t *s)
{
    return jl_string_data(s);
}

DLLEXPORT jl_value_t *jl_call1(jl_function_t *f, jl_value_t *a)
{
    jl_value_t *v;
    JL_TRY {
        JL_GC_PUSH2(&f,&a);
        v = jl_apply(f, &a, 1);
        JL_GC_POP();
    }
    JL_CATCH {
        v = NULL;
    }
    return v;
}

DLLEXPORT jl_value_t *jl_call2(jl_function_t *f, jl_value_t *a, jl_value_t *b)
{
    jl_value_t *v;
    JL_TRY {
        JL_GC_PUSH3(&f,&a,&b);
        jl_value_t *args[2] = {a,b};
        v = jl_apply(f, args, 2);
        JL_GC_POP();
    }
    JL_CATCH {
        v = NULL;
    }
    return v;
}

JL_CALLABLE(jl_f_get_field);
DLLEXPORT jl_value_t *jl_get_field(jl_value_t *o, char *fld)
{
    jl_value_t *v;
    JL_TRY {
        jl_value_t *s = (jl_value_t*)jl_symbol(fld);
        jl_value_t *args[2] = {o, s};
        v = jl_f_get_field(NULL, args, 2);
    }
    JL_CATCH {
        v = NULL;
    }
    return v;
}

DLLEXPORT void jl_sigatomic_begin(void)
{
    JL_SIGATOMIC_BEGIN();
}

DLLEXPORT void jl_sigatomic_end(void)
{
    if (jl_defer_signal == 0)
        jl_error("sigatomic_end called in non-sigatomic region");
    JL_SIGATOMIC_END();
}

DLLEXPORT int jl_is_debugbuild(void) {
#ifdef DEBUG
    return 1;
#else
    return 0;
#endif
}

