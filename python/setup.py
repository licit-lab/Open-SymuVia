from setuptools import setup, Extension, find_packages
# from distutils.extension import Extension
from Cython.Build import cythonize, build_ext
import os, sys, shutil, glob

symuvia_src = os.path.abspath('../')

print('='*20, 'INSTALLING HEADERS SymuCore', '='*20)

HEADERS_SYMUCORE = ['Graph/Model/Graph.h',
                    'SymuCoreExports.h',
                    'Utils/SymuCoreConstants.h']


symucore_install = os.path.expandvars('$CONDA_PREFIX/include/symucore')


if not os.path.exists(symucore_install):
    os.mkdir(symucore_install)
    os.mkdir(os.path.join(symucore_install, 'Utils'))
    os.mkdir(os.path.join(symucore_install, 'Graph'))
    os.mkdir(os.path.join(symucore_install, 'Graph/Model'))

for file in HEADERS_SYMUCORE:
    print('INSTALLING HEADER:', file, '->', os.path.join(symucore_install, file))
    shutil.copy(os.path.join(symuvia_src, 'symucore/src', file), os.path.join(symucore_install, file))

sources = glob.glob('src/pysymuvia/symucore/**/*.pyx', recursive = True)
print('SOURCES:', sources)

symucore = Extension(
    name="pysymuvia.symucore.graph",
    sources=sources,
    libraries=["SymuCore"],
    library_dirs=[os.path.expandvars('$CONDA_PREFIX/lib')],
    include_dirs=[os.path.expandvars('$CONDA_PREFIX/include/symucore')],
)

print('='*70, '\n')

print('='*20, 'INSTALLING HEADERS SymuMaster', '='*20)

HEADERS_SYMUMASTER = ['SymuMasterExports.h',
                      'Simulation/Config/SimulatorConfiguration.h',
                      'Simulation/Config/SimulationConfiguration.h',
                      "Launcher/ColouredConsoleLogger.h",
                      "IO/XMLLoader.h",
                      "IO/XMLStringUtils.h",
                      "Utils/SystemUtils.h",
                      'Utils/SymuMasterConstants.h',
                      "Simulation/Config/SimulatorConfigurationFactory.h",
                      "Simulation/Assignment/AssignmentModel.h",
                      "Simulation/Assignment/AssignmentModelFactory.h"]

symumaster_install = os.path.expandvars('$CONDA_PREFIX/include/symumaster')


if not os.path.exists(symumaster_install):
    os.mkdir(symumaster_install)
    os.mkdir(os.path.join(symumaster_install, 'IO'))
    os.mkdir(os.path.join(symumaster_install, 'Simulation'))
    os.mkdir(os.path.join(symumaster_install, 'Launcher'))
    os.mkdir(os.path.join(symumaster_install, 'Utils'))
    os.mkdir(os.path.join(symumaster_install, 'Simulation/Config'))
    os.mkdir(os.path.join(symumaster_install, 'Simulation/Assignment'))

for file in HEADERS_SYMUMASTER:
    print('INSTALLING HEADER:', file, '->', os.path.join(symumaster_install, file))
    shutil.copy(os.path.join(symuvia_src, 'symumaster', file), os.path.join(symumaster_install, file))

sources = glob.glob('src/pysymuvia/symumaster/**/*.pyx', recursive = True)
print('SOURCES:', sources)

symucore = Extension(
    name="pysymuvia.symumaster.config",
    sources=sources,
    libraries=["SymuMaster"],
    library_dirs=[os.path.expandvars('$CONDA_PREFIX/lib')],
    include_dirs=[os.path.expandvars('$CONDA_PREFIX/include/symumaster'), os.path.expandvars('$CONDA_PREFIX/include/symucore')]
)

print('='*70)



if os.path.exists('build'):
    print('REMOVING build/ and *.cpp')
    shutil.rmtree('build')
    [os.remove(filename) for filename in os.listdir('.') if '.cpp' in filename]


setup(
    name="pysymuvia",
    ext_modules=cythonize([symucore]),
    packages=find_packages('src'),
    package_dir={'': 'src'},
    zip_safe=False,
    cmdclass={"build_ext" : build_ext},
)
