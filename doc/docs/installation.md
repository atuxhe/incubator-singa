#Building SINGA from source

## Dependencies

### Required
* Google Protobuf (>=2.5)
* BLAS (tested with OpenBLAS >=0.2.10)
* CUDA (tested with 6.5, 7.0 and 7.5)
* CUDNN (v4 and v5)

Uses must install the above mandatory libraries.
Currently CUDA and CUNN are also mandatory, but it would become optional later.

### Optional
* Glog
* OpenCV (tested with 2.4.8)
* LMDB (tested with 0.9)


## Instructions

Please clone the newest code from [Github](https://github.com/apache/incubator-singa) and execute the following commands,


    $ git clone https://github.com/apache/incubator-singa.git
    $ cd incubator-singa/
    # switch to dev branch
    $ git checkout dev


If you use CUDA, then [CNMeM](https://github.com/NVIDIA/cnmem) is necessary,
which could be downloaded as

    $ git submodule init
    $ git submodule update


### Linux OS

Then in SINGA_ROOT, execute the following commands for compiling SINGA,

    $ mkdir build && cd build
    # generate Makefile for compilation
    $ cmake ..
    # compile SINGA
    $ make

You can use `ccmake ..` to configure the compilation options including using
LMDB, GLOG, etc.

After compiling SINGA, you can run the unit tests by

    $ ./bin/test_singa

You can see all the testing cases with testing results. If SINGA passes all
tests, then you have successfully installed SINGA. Please proceed to enjoy SINGA!


### MacOS


### Windows