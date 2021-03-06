#ifndef JULIA_API_H
#define JULIA_API_H
#include "julia.h"

extern char *julia_home;
static const char *jlapi_mode; 

//No header jlapi.h in julia => added here locally 
DLLEXPORT char *jl_locate_sysimg(char *jlhome);
DLLEXPORT void *jl_eval_string(char *str);
DLLEXPORT void jl_init(char *julia_home_dir);
DLLEXPORT const char *jl_typename_str(jl_value_t *v);
DLLEXPORT const char *jl_typeof_str(jl_value_t *v);
DLLEXPORT const char *jl_bytestring_ptr(jl_value_t *s);
DLLEXPORT void *jl_array_eltype(jl_value_t *a);
DLLEXPORT int jl_array_rank(jl_value_t *a);
DLLEXPORT size_t jl_array_size(jl_value_t *a, int d);
//static size_t array_nd_index(jl_array_t *a, jl_value_t **args, size_t nidxs, char *fname);
DLLEXPORT jl_value_t *jl_get_field(jl_value_t *o, char *fld);
DLLEXPORT jl_value_t *jl_exception_occurred(void);
DLLEXPORT void jl_exception_clear(void);
DLLEXPORT int load_library_permanently(const char *libname);

/************* Tools ********************/

DLLEXPORT void jlapi_init(char *julia_home_dir, char* mode);
DLLEXPORT const char *jlapi_get_stdout();
DLLEXPORT const char *jlapi_get_stderr();

//-| Print STDOUT, STDERR IOBuffer
DLLEXPORT void jlapi_print_stdout();

DLLEXPORT void jlapi_print_stderr();

DLLEXPORT jl_value_t *jlapi_box_bool(int8_t x);

DLLEXPORT jl_value_t *jlapi_box_float64(double x);

#endif