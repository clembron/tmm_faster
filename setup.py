import os
import sys
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11
import numpy as np

class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific flags."""
    def build_extensions(self):
        opts = []
        link_opts = []
        
        if self.compiler.compiler_type == 'msvc':  # Windows
            opts += ['/openmp', '/O2', '/std:c++17']
            link_opts += ['/DEFAULTLIB:vcomp.lib']
        else:  # Linux / macOS (GCC oder Clang)
            opts += ['-fopenmp', '-O3', '-std=c++17']
            link_opts += ['-fopenmp']
        
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
            ext.include_dirs.append(pybind11.get_include())
            ext.include_dirs.append(np.get_include())
            
        super().build_extensions()

ext_modules = [
    Extension(
        "tmm_faster.tmm_faster_cpp",
        sources=[
            "src/TMMCore.cpp",
            "src/TMMWrapper.cpp"
        ],
        language='c++',
    ),
]

setup(
    name="tmm_faster",
    ext_modules=ext_modules,
    cmdclass={'build_ext': BuildExt},
    packages=["tmm_faster"],
    package_data={
        "tmm_faster": ["*.pyi"],
    },
    include_package_data=True,
    zip_safe=False
)