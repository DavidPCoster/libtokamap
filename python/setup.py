from setuptools import setup, Extension
import numpy

ext_modules = [
    Extension(
        "clibtokamap",
        sources=["libtokamap.cpp"],
        include_dirs=["../include", "../src", "../ext_include", "../build/include", numpy.get_include()],
        extra_objects = ["../build/libtokamap.a"],
        extra_compile_args = ["-std=c++20", "-O0"],
    )
]

setup(
    name="libtokamap",
    version="0.1.0",
    ext_modules=ext_modules,
)
