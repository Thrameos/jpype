from setuptools import setup, Extension

module = Extension(
    "helloworld",  # Name of the module
    sources=["helloworld.c"]  # Source file
)

setup(
    name="helloworld",
    version="1.0",
    description="A simple Hello, World module",
    ext_modules=[module],
)
