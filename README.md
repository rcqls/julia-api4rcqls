# Extension of julia API for RCqls

There is an official julia api (see src/jlapi.c) provided in the Github julia project. This personal project is mainly devoted to
develop some extension  of the official jlapic.c without interfering with the official one. Indeed, since julia is a very active project, there is many directions in the development of  such API.

# How to use it?

This is just a complementary folder to place at the same level as the source julia directory. 
After, compiling the julia source directory, a make command in this folder provides libjulia-api.(so|dylib) and julia-api.h  (and temporarily, src/support/htableh.inc which is apparently needed) files directly copied to the current JULIA_COMMIT folder.

# jl4R and jl4rb projects

In fact, this julia api is primarily intended to make easier the very experimental development of both [jl4R](https://github.com/rcqls/jl4R) and [jl4rb](https://github.com/rcqls/jl4rb) projects
