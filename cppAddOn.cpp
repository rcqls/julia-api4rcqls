#include "llvm/Support/DynamicLibrary.h"
#include <stdio.h>

using namespace llvm;

extern "C"  void load_library_permanently(const char *libname)
{

    if(sys::DynamicLibrary::LoadLibraryPermanently(libname)) {
    	printf("%s not loaded\n",libname);
    } else {
    	printf("%s loaded\n",libname);
    };
}