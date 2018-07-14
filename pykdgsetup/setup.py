from setuptools import setup, Extension, find_packages
import os, os.path
from os.path import abspath, join

root_path = os.path.abspath('../')
module1 = Extension('pykdgc',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0')],
                    include_dirs = [join(root_path,'kdg/include/')],
                    libraries = ['kdg'],
                    library_dirs = [join(root_path, 'kdg/')],
                    sources = ['src/pykdgmodule.c'],
                    runtime_library_dirs = [join(root_path, 'kdg/')])

setup (name = 'pykdg',
       version = '1.0',
       description = 'no',
       author = 'kz',
       author_email = 'no',
       url = 'no',
       long_description = '''
       no
       no, no
''',
       packages=find_packages() ,
       # py_modules=[],
       ext_modules = [module1])
