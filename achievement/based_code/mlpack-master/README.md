<h2 align="center">
  <a href="http://mlpack.org"><img
src="https://cdn.rawgit.com/mlpack/mlpack.org/e7d36ed8/mlpack-black.svg" style="background-color:rgba(0,0,0,0);" height=230 alt="mlpack: a fast, flexible machine learning library"></a>
  <br>a fast, flexible machine learning library<br>
</h2>

<h5 align="center">
  <a href="http://mlpack.org">Home</a> |
  <a href="http://www.mlpack.org/docs/mlpack-git/doxygen/index.html">Documentation</a> |
  <a href="http://www.mlpack.org/involved.html">Community</a> |
  <a href="http://www.mlpack.org/help.html">Help</a> |
  <a href="http://webchat.freenode.net/?channels=mlpack">IRC Chat</a>
</h5>

<p align="center">
  <a href="http://masterblaster.mlpack.org/job/mlpack%20-%20git%20commit%20test/"><img src="https://img.shields.io/jenkins/s/http/masterblaster.mlpack.org/job/mlpack%20-%20git%20commit%20test.svg?label=Linux%20build&style=flat-square" alt="Jenkins"></a>
  <a href="https://ci.appveyor.com/project/mlpack/mlpack"><img src="https://img.shields.io/appveyor/ci/mlpack/mlpack/master.svg?label=Windows%20build&style=flat-square&logoWidth=0.1" alt="Appveyor"></a>
  <a href="https://coveralls.io/github/mlpack/mlpack?branch=master"><img src="https://img.shields.io/coveralls/mlpack/mlpack/master.svg?style=flat-square" alt="Coveralls"></a>
  <a href="https://opensource.org/licenses/BSD-3-Clause"><img src="https://img.shields.io/badge/License-BSD%203--Clause-blue.svg?style=flat-square" alt="License"></a>
</p>

<p align="center">
  <em>
    Download:
    <a href="http://www.mlpack.org/files/mlpack-3.0.0.tar.gz">current stable version (3.0.0)</a>
  </em>
</p>

**mlpack** is an intuitive, fast, and flexible C++ machine learning library with
bindings to other languages.  It is meant to be a machine learning analog to
LAPACK, and aims to implement a wide array of machine learning methods and
functions as a "swiss army knife" for machine learning researchers.  In addition
to its powerful C++ interface, mlpack also provides command-line programs and
Python bindings.

### 0. Contents

  1. [Introduction](#1-introduction)
  2. [Citation details](#2-citation-details)
  3. [Dependencies](#3-dependencies)
  4. [Building mlpack from source](#4-building-mlpack-from-source)
  5. [Running mlpack programs](#5-running-mlpack-programs)
  6. [Using mlpack from Python](#6-using-mlpack-from-python)
  7. [Further documentation](#7-further-documentation)
  8. [Bug reporting](#8-bug-reporting)

###  1. Introduction

The mlpack website can be found at http://www.mlpack.org and contains numerous
tutorials and extensive documentation.  This README serves as a guide for what
mlpack is, how to install it, how to run it, and where to find more
documentation. The website should be consulted for further information:

  - [mlpack homepage](http://www.mlpack.org/)
  - [Tutorials](http://www.mlpack.org/docs/mlpack-git/doxygen/tutorials.html)
  - [Development Site (Github)](http://www.github.com/mlpack/mlpack/)
  - [API documentation](http://www.mlpack.org/docs/mlpack-git/doxygen/index.html)

### 2. Citation details

If you use mlpack in your research or software, please cite mlpack using the
citation below (given in BiBTeX format):

    @article{mlpack2013,
      title     = {{mlpack}: A Scalable {C++} Machine Learning Library},
      author    = {Curtin, Ryan R. and Cline, James R. and Slagle, Neil P. and
                   March, William B. and Ram, P. and Mehta, Nishant A. and Gray,
                   Alexander G.},
      journal   = {Journal of Machine Learning Research},
      volume    = {14},
      pages     = {801--805},
      year      = {2013}
    }

Citations are beneficial for the growth and improvement of mlpack.

### 3. Dependencies

mlpack has the following dependencies:

      Armadillo     >= 6.500.0
      Boost (program_options, math_c99, unit_test_framework, serialization,
             spirit)
      CMake         >= 2.8.5

All of those should be available in your distribution's package manager.  If
not, you will have to compile each of them by hand.  See the documentation for
each of those packages for more information.

If you would like use or build the mlpack Python bindings, make sure that the
following Python packages are installed:

      setuptools
      cython >= 0.24
      numpy
      pandas >= 0.15.0

If you are compiling Armadillo by hand, ensure that LAPACK and BLAS are enabled.

### 4. Building mlpack from source

This section discusses how to build mlpack from source.  However, mlpack is in
the repositories of many Linux distributions and so it may be easier to use the
package manager for your system.  For example, on Ubuntu, you can install mlpack
with the following command:

    $ sudo apt-get install libmlpack-dev

There are some other useful pages to consult in addition to this section:

  - [Building mlpack From Source](http://www.mlpack.org/docs/mlpack-git/doxygen/build.html)
  - [Building mlpack Under Windows](https://github.com/mlpack/mlpack/wiki/WindowsBuild)

mlpack uses CMake as a build system and allows several flexible build
configuration options. One can consult any of numerous CMake tutorials for
further documentation, but this tutorial should be enough to get mlpack built
and installed.

First, unpack the mlpack source and change into the unpacked directory.  Here we
use mlpack-x.y.z where x.y.z is the version.

    $ tar -xzf mlpack-x.y.z.tar.gz
    $ cd mlpack-x.y.z

Then, make a build directory.  The directory can have any name, not just
'build', but 'build' is sufficient.

    $ mkdir build
    $ cd build

The next step is to run CMake to configure the project.  Running CMake is the
equivalent to running `./configure` with autotools. If you run CMake with no
options, it will configure the project to build with no debugging symbols and no
profiling information:

    $ cmake ../

You can specify options to compile with debugging information and profiling
information:

    $ cmake -D DEBUG=ON -D PROFILE=ON ../

Options are specified with the -D flag.  A list of options allowed:

    DEBUG=(ON/OFF): compile with debugging symbols
    PROFILE=(ON/OFF): compile with profiling symbols
    ARMA_EXTRA_DEBUG=(ON/OFF): compile with extra Armadillo debugging symbols
    BOOST_ROOT=(/path/to/boost/): path to root of boost installation
    ARMADILLO_INCLUDE_DIR=(/path/to/armadillo/include/): path to Armadillo headers
    ARMADILLO_LIBRARY=(/path/to/armadillo/libarmadillo.so): Armadillo library
    BUILD_CLI_EXECUTABLES=(ON/OFF): whether or not to build command-line programs
    BUILD_PYTHON_BINDINGS=(ON/OFF): whether or not to build Python bindings

Other tools can also be used to configure CMake, but those are not documented
here.

By default, command-line programs will be built, and if the Python dependencies
(Cython, setuptools, numpy, pandas) are available, then Python bindings will
also be built.

Once CMake is configured, building the library is as simple as typing 'make'.
This will build all library components as well as 'mlpack_test'.

    $ make

You can specify individual components which you want to build, if you do not
want to build everything in the library:

    $ make mlpack_pca mlpack_knn mlpack_kfn

If the build fails and you cannot figure out why, register an account on Github
and submit an issue; the mlpack developers will quickly help you figure it out:

[mlpack on Github](https://www.github.com/mlpack/mlpack/)

Alternately, mlpack help can be found in IRC at `#mlpack` on irc.freenode.net.

If you wish to install mlpack to `/usr/local/include/mlpack/` and `/usr/local/lib/`
and `/usr/local/bin/`, once it has built, make sure you have root privileges (or
write permissions to those three directories), and simply type

    $ make install

You can now run the executables by name; you can link against mlpack with
    `-lmlpack`
and the mlpack headers are found in
    `/usr/local/include/mlpack/`
and if Python bindings were built, they will be accessible with the `mlpack`
package in Python.

If running the programs (i.e. `$ mlpack_knn -h`) gives an error of the form

    error while loading shared libraries: libmlpack.so.2: cannot open shared object file: No such file or directory

then be sure that the runtime linker is searching the directory where
`libmlpack.so` was installed (probably `/usr/local/lib/` unless you set it
manually).  One way to do this, on Linux, is to ensure that the
`LD_LIBRARY_PATH` environment variable has the directory that contains
`libmlpack.so`.  Using bash, this can be set easily:

    export LD_LIBRARY_PATH="/usr/local/lib/:$LD_LIBRARY_PATH"

(or whatever directory `libmlpack.so` is installed in.)

### 5. Running mlpack programs

After building mlpack, the executables will reside in `build/bin/`.  You can call
them from there, or you can install the library and (depending on system
settings) they should be added to your PATH and you can call them directly.  The
documentation below assumes the executables are in your PATH.

Consider the 'mlpack_knn' program, which finds the k nearest neighbors in a
reference dataset of all the points in a query set.  That is, we have a query
and a reference dataset. For each point in the query dataset, we wish to know
the k points in the reference dataset which are closest to the given query
point.

Alternately, if the query and reference datasets are the same, the problem can
be stated more simply: for each point in the dataset, we wish to know the k
nearest points to that point.

Each mlpack program has extensive help documentation which details what the
method does, what each of the parameters are, and how to use them:

```shell
$ mlpack_knn --help
```

Running `mlpack_knn` on one dataset (that is, the query and reference
datasets are the same) and finding the 5 nearest neighbors is very simple:

```shell
$ mlpack_knn -r dataset.csv -n neighbors_out.csv -d distances_out.csv -k 5 -v
```

The `-v (--verbose)` flag is optional; it gives informational output.  It is not
unique to `mlpack_knn` but is available in all mlpack programs.  Verbose
output also gives timing output at the end of the program, which can be very
useful.

### 6. Using mlpack from Python

If mlpack is installed to the system, then the mlpack Python bindings should be
automatically in your PYTHONPATH, and importing mlpack functionality into Python
should be very simple:

```python
>>> from mlpack import knn
```

Accessing help is easy:

```python
>>> help(knn)
```

The API is similar to the command-line programs.  So, running `knn()`
(k-nearest-neighbor search) on the numpy matrix `dataset` and finding the 5
nearest neighbors is very simple:

```python
>>> output = knn(reference=dataset, k=5, verbose=True)
```

This will store the output neighbors in `output['neighbors']` and the output
distances in `output['distances']`.  Other mlpack bindings function similarly,
and the input/output parameters exactly match those of the command-line
programs.

### 7. Further documentation

The documentation given here is only a fraction of the available documentation
for mlpack.  If doxygen is installed, you can type `make doc` to build the
documentation locally.  Alternately, up-to-date documentation is available for
older versions of mlpack:

  - [mlpack homepage](http://www.mlpack.org/)
  - [Tutorials](http://www.mlpack.org/docs/mlpack-git/doxygen/tutorials.html)
  - [Development Site (Github)](https://www.github.com/mlpack/mlpack/)
  - [API documentation](http://www.mlpack.org/docs/mlpack-git/doxygen/index.html)

### 8. Bug reporting

   (see also [mlpack help](http://www.mlpack.org/help.html))

If you find a bug in mlpack or have any problems, numerous routes are available
for help.

Github is used for bug tracking, and can be found at
https://github.com/mlpack/mlpack/.
It is easy to register an account and file a bug there, and the mlpack
development team will try to quickly resolve your issue.

In addition, mailing lists are available.  The mlpack discussion list is
available at

  [mlpack discussion list](http://lists.mlpack.org/mailman/listinfo/mlpack)

and the git commit list is available at

  [commit list](http://lists.mlpack.org/mailman/listinfo/mlpack-git)

Lastly, the IRC channel `#mlpack` on Freenode can be used to get help.
