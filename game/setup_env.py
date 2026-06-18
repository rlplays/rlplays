# Copied from thirdparty/PufferLib/setup.py
'''
Setup script for RLPlays game + PufferLib using a copy of setup.py from PufferLib.
'''


from setuptools import find_packages, find_namespace_packages, setup, Extension
import numpy
import os
import glob
import urllib.request
import zipfile
import tarfile
import platform
import shutil

import pybind11

from setuptools.command.build_ext import build_ext
import sys
import torch
from torch.utils import cpp_extension
from torch.utils.cpp_extension import (
    CppExtension,
    CUDAExtension,
    BuildExtension,
    CUDA_HOME,
    ROCM_HOME
)

# build cuda extension if torch can find CUDA or HIP/ROCM in the system
# may require `uv pip install --no-build-isolation` or `python setup.py build_ext --inplace`
BUILD_CUDA_EXT = bool(CUDA_HOME or ROCM_HOME)

# Build with DEBUG=1 to enable debug symbols
DEBUG = os.getenv("DEBUG", "0") == "1"
NO_OCEAN = os.getenv("NO_OCEAN", "0") == "1"
NO_TRAIN = os.getenv("NO_TRAIN", "0") == "1"
NO_TORCH = os.getenv("NO_TORCH", "0") == "1"
NO_ASAN = os.getenv("NO_ASAN", "0") == "1"
SINGLE_THREADED = os.getenv("SINGLE_THREADED", "0") == "1"
SELF_PLAY = os.getenv("SELF_PLAY", "0") == "1"

# Enable parallel compilation if not already set
if not os.environ.get("MAX_JOBS"):
    os.environ["MAX_JOBS"] = str(os.cpu_count() or 1)

print(f"------- DEBUG MODE? {DEBUG} | SELF_PLAY? {SELF_PLAY} | MAX_JOBS={os.environ['MAX_JOBS']} -------------")
if SINGLE_THREADED:
    print("------- SINGLE THREADED MODE! -------------")

# Build raylib for your platform
RAYLIB_URL = 'https://github.com/raysan5/raylib/releases/download/5.5/'
RAYLIB_NAME = 'raylib-5.5_macos' if platform.system() == "Darwin" else 'raylib-5.5_linux_amd64'
RLIGHTS_URL = 'https://raw.githubusercontent.com/raysan5/raylib/refs/heads/master/examples/shaders/rlights.h'

def download_raylib(platform, ext):
    platform = f'thirdparty/PufferLib/{platform}'
    if not os.path.exists(platform):
        print(f'Downloading Raylib {platform}')
        urllib.request.urlretrieve(RAYLIB_URL + platform + ext, platform + ext)
        if ext == '.zip':
            with zipfile.ZipFile(platform + ext, 'r') as zip_ref:
                zip_ref.extractall()
        else:
            with tarfile.open(platform + ext, 'r') as tar_ref:
                tar_ref.extractall()

        os.remove(platform + ext)
        urllib.request.urlretrieve(RLIGHTS_URL, platform + '/include/rlights.h')

if not NO_OCEAN:
    download_raylib('raylib-5.5_webassembly', '.zip')

# Shared compile args for all platforms
extra_compile_args = [
    '-DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION',
    '-DPLATFORM_DESKTOP',
    # For training, we enforce headless and no scene transitions as want the highest SPS possible.
    '-DRLPLAYS_TRAIN',
    '-DRLPLAYS_HIDE_SCENE_TRANSITIONS',
    '-DRLPLAYS_HEADLESS',
    '-DPUFFER_NATIVECPP_PYBINDINGS',
    '-std=gnu++20',
    '-fpermissive',
]

CUDA_INCLUDE = []
if CUDA_HOME:
    CUDA_INCLUDE.append(os.path.join(CUDA_HOME, "include"))
    print(f"Adding CUDA include path: {CUDA_INCLUDE[-1]}")

if DEBUG:
    extra_compile_args += [
        '-DDEBUG',
    ]

extra_link_args = [
    '-fwrapv',
]
cxx_args = [
    '-fdiagnostics-color=always',
    '-std=gnu++20',
    '-fpermissive',
]
nvcc_args = []

if DEBUG:
    extra_compile_args += [
        '-O0',
        '-g',
        '-fno-omit-frame-pointer',
    ]
    extra_link_args += [
        '-g',
    ]
    cxx_args += [
        '-O0',
        '-g',
    ]
    nvcc_args += [
        '-O0',
        '-g',
    ]
    if not NO_ASAN:
      extra_compile_args += [
          '-fsanitize=address,undefined,bounds,pointer-overflow,leak',
      ]
      extra_link_args += [
          '-fsanitize=address,undefined,bounds,pointer-overflow,leak',
      ]
else:
    extra_compile_args += [
        '-O3',
        '-flto',
    ]
    extra_link_args += [
        '-O3',
    ]
    cxx_args += [
        '-O3',
    ]
    nvcc_args += [
        '-O3',
    ]


if SELF_PLAY:
    print("Building with self-play support (defining PUFFERLIB_SELFPLAY)")
    extra_compile_args += [
        '-DPUFFERLIB_SELFPLAY',
    ]

if SINGLE_THREADED:
    print('Enabling single-theaded mode (defining PUFFER_SINGLE_THREADED)')
    extra_compile_args += [
        '-DPUFFER_SINGLE_THREADED',
    ]    

system = platform.system()
if system == 'Linux':
    extra_compile_args += [
        '-Wno-alloc-size-larger-than',
        '-Wno-odr', # One definition rule - C/C++ messiness
        '-Wno-attributes', # pybind11 
        '-Wno-unknown-pragmas', # Win VS vs Linux stuff
        # '-Wno-implicit-function-declaration', # Ignored, it's C++ not C, it's an error already.
        '-fmax-errors=10',
    ]
    extra_link_args += [
        '-Bsymbolic-functions',
    ]
    if not NO_OCEAN:
        download_raylib('raylib-5.5_linux_amd64', '.tar.gz')
elif system == 'Darwin':
    extra_compile_args += [
        '-Wno-error=int-conversion',
        '-Wno-error=incompatible-function-pointer-types',
        '-Wno-error=implicit-function-declaration',
    ]
    extra_link_args += [
        '-framework', 'Cocoa',
        '-framework', 'OpenGL',
        '-framework', 'IOKit',
    ]
    if not NO_OCEAN:
        download_raylib('raylib-5.5_macos', '.tar.gz')
else:
    raise ValueError(f'Unsupported system: {system}')

# Default Gym/Gymnasium/PettingZoo versions
# Gym:
# - 0.26 still has deprecation warnings and is the last version of the package
# - 0.25 adds a breaking API change to reset, step, and render_modes
# - 0.24 is broken
# - 0.22-0.23 triggers deprecation warnings by calling its own functions
# - 0.21 is the most stable version
# - <= 0.20 is missing dict methods for gym.spaces.Dict
# - 0.18-0.21 require setuptools<=65.5.0

GYMNASIUM_VERSION = '0.29.1'
GYM_VERSION = '0.23'
PETTINGZOO_VERSION = '1.24.1'


docs = [
    'sphinx==5.0.0',
    'sphinx-rtd-theme==0.5.1',
    'sphinxcontrib-youtube==1.0.1',
    'sphinx-rtd-theme==0.5.1',
    'sphinx-design==0.4.1',
    'furo==2023.3.27',
]

ray = [
    'ray==2.23.0',
]

cleanrl = [
    'stable_baselines3==2.1.0',
    'tensorboard==2.11.2',
    'tyro==0.8.6',
]


# Extensions 
class BuildExt(build_ext):
    def run(self):
        # Propagate any build_ext options (e.g., --inplace, --force) to subcommands
        build_ext_opts = self.distribution.command_options.get('build_ext', {})
        if build_ext_opts:
            # Copy flags so build_torch and build_c respect inplace/force
            self.distribution.command_options['build_torch'] = build_ext_opts.copy()
            self.distribution.command_options['build_c'] = build_ext_opts.copy()

        # Run the torch and C builds (which will handle copying when inplace is set)
        if not NO_TORCH:        
            self.run_command('build_torch')
        self.run_command('build_c')

class CBuildExt(build_ext):
    def run(self, *args, **kwargs):
        self.parallel = int(os.environ.get("MAX_JOBS", 1))
        self.extensions = [e for e in self.extensions if not (e.name == "thirdparty.PufferLib.pufferlib._C" or e.name == "thirdparty.PufferLib.pufferlib.native")]
        for e in self.extensions:
            print (f'Adding C ext: {e}')
        super().run(*args, **kwargs)

class TorchBuildExt(cpp_extension.BuildExtension):
    def run(self):
        self.extensions = [e for e in self.extensions if (e.name == "thirdparty.PufferLib.pufferlib._C" or e.name == "thirdparty.PufferLib.pufferlib.native")]
        for e in self.extensions:
            print (f'Adding Torch ext: {e}')
        super().run()

RAYLIB_A = 'build/_deps/raylib-build/raylib/libraylib.a'
coreloop_sources = glob.glob('coreloop/src/*.cpp') + glob.glob('plays/*.cpp')
INCLUDE = [numpy.get_include(), 
           'raylib/include', 
           'thirdparty/raylib/src', 
           'thirdparty/jsoncpp/include',
           'thirdparty/raylib/src/external/glfw/include',
           'thirdparty/PufferLib/pufferlib/extensions',
           'coreloop/include',
           'rlplays/include',
           'plays/'
           ] + CUDA_INCLUDE
raylib_lib_dir = os.path.abspath(f'thirdparty/PufferLib/{RAYLIB_NAME}/lib')
torch_lib_dirs = torch.utils.cpp_extension.library_paths()
all_lib_dirs = torch_lib_dirs + [raylib_lib_dir]
all_rpaths = [f'-Wl,-rpath,{path}' for path in torch_lib_dirs + [raylib_lib_dir]]
extension_kwargs = dict(
    include_dirs=INCLUDE,
    library_dirs=all_lib_dirs,
    libraries=['torch', 'torch_cpu', 'c10', 'raylib'],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args + all_rpaths,
    extra_objects=[f'build/lib.linux-x86_64-cpython-{sys.version_info.major}{sys.version_info.minor}/thirdparty/PufferLib/pufferlib/native.cpython-{sys.version_info.major}{sys.version_info.minor}-x86_64-linux-gnu.so'],
)

# TODO: Include other C files so rebuild is auto?
c_extensions = []

if not NO_OCEAN:
    c_extension_paths = ['rlplays/binding.cpp']

    c_extensions = [
        CppExtension(
            path.rstrip('.cpp').replace('/', '.'),
            sources=[path] + coreloop_sources + ['thirdparty/PufferLib/pufferlib/ocean/puffer_native_eval.cpp'],
            language='c++',
            **extension_kwargs,
        )
        for path in c_extension_paths
    ]
    print(f'Found {len(c_extensions)} C extensions to build')

    for ext in c_extensions:
        print(f'Adding extension: {ext.name} with sources: {ext.sources} / {ext.include_dirs}')
    for ext in c_extension_paths:
        print(f'Adding extension path: {ext}')
    c_extension_paths = ['rlplays']


# Check if CUDA compiler is available. You need cuda dev, not just runtime.
torch_extensions = []
if not NO_TRAIN:
    torch_sources = [
        "thirdparty/PufferLib/pufferlib/extensions/pufferlib.cpp",
    ]
    if BUILD_CUDA_EXT:
        extension = CUDAExtension
        torch_sources += ["thirdparty/PufferLib/pufferlib/extensions/cuda/pufferlib.cu"]
        torch_extensions += [
           extension(
                "thirdparty.PufferLib.pufferlib.native",
                ["thirdparty/PufferLib/pufferlib/puffer_cuda_kernels.cu"],
                extra_compile_args = {
                    "cxx": cxx_args,
                    "nvcc": nvcc_args,
                }
            ),
        ]
    else:
        extension = CppExtension

    torch_extensions += [
       extension(
            "thirdparty.PufferLib.pufferlib._C",
            torch_sources,
            extra_compile_args = {
                "cxx": cxx_args,
                "nvcc": nvcc_args,
            }
        ),
    ]

# Prevent Conda from injecting garbage compile flags
from distutils.sysconfig import get_config_vars
cfg_vars = get_config_vars()

for key, value in cfg_vars.items():
    if value and '-fno-strict-overflow' in str(value):
        cfg_vars[key] = value.replace('-fno-strict-overflow', '')

install_requires = [
    'numpy<2.0',
    f'gym<={GYM_VERSION}',
    f'gymnasium<={GYMNASIUM_VERSION}',
    f'pettingzoo<={PETTINGZOO_VERSION}',
    'shimmy[gym-v21]',
    'setuptools'
]

if not NO_TRAIN:
    install_requires += [
        'torch',
        'psutil',
        'nvidia-ml-py',
        'rich',
        'rich_argparse',
        'imageio',
        'pyro-ppl',
        'heavyball',
        'neptune',
        'wandb',
    ]

setup(
    name="thirdparty.PufferLib.pufferlib",
    version="3.0.0",
    long_description_content_type="text/markdown",
    packages=find_namespace_packages() + find_packages() + c_extension_paths + ['thirdparty/PufferLib/pufferlib/extensions'],
    package_data={
        "pufferlib": [f'thirdparty/PufferLib/{RAYLIB_NAME}/lib/libraylib.a']
    },
    include_package_data=True,
    install_requires=install_requires,
    extras_require={
        'docs': docs,
        'ray': ray,
        'cleanrl': cleanrl,
    },
    ext_modules = c_extensions + torch_extensions,
    cmdclass={
        "build_ext": BuildExt,
        "build_torch": TorchBuildExt,
        "build_c": CBuildExt,
    },
    include_dirs=[numpy.get_include(), f'thirdparty/PufferLib/{RAYLIB_NAME}/include',
                  f'thirdparty/PufferLib/pufferlib/ocean',
                  pybind11.get_include(), ],
    entry_points={
        'console_scripts': [
            'puffer = thirdparty.PufferLib.pufferlib.pufferl:main',
        ],
    },
)
