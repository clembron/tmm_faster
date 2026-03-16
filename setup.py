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
        else:  # Linux / macOS
            opts += ['-O3', '-std=c++17']
            
            if sys.platform == "darwin":
                omp_prefix = "/opt/homebrew/opt/libomp" if os.path.exists("/opt/homebrew/opt/libomp") else "/usr/local/opt/libomp"
                
                if os.path.exists(omp_prefix):
                    opts += ['-Xpreprocessor', '-fopenmp', f'-I{omp_prefix}/include']
                    link_opts += [f'-L{omp_prefix}/lib', '-lomp']
                else:
                    print("Warning: libomp not found, building without OpenMP")
            else:
                opts += ['-fopenmp']
                link_opts += ['-fopenmp']
        
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
            ext.include_dirs = [
                pybind11.get_include(),
                np.get_include(),
                "src"
            ]
            
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
    packages=["tmm_faster"],
    ext_modules=ext_modules,
    cmdclass={'build_ext': BuildExt},
)
