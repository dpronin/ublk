# Project building

## Build requirements

- [cmake](https://cmake.org/) to configure the project. Minimum required version is __3.19__ if [_cmake presets_](https://cmake.org/cmake/help/v3.19/manual/cmake-presets.7.html) are going to be used, otherwise __3.12__ would do
- [conan 2.0](https://conan.io/) to download all the dependencies of the application and configure with a certain set of parameters. You can install __conan__ by giving a command to __pip__. To use __pip__ you need to install __python__ interpreter. I highly recommend to install a __python3__-based version and as the result use __pip3__ in order to avoid unexpected results with __conan__

To install/upgrade __conan__ within system's python environment for a current linux's user give the command:

```bash
bash> pip3 install --user conan --upgrade
```

- A C++ compiler with at least __C++23__ support

## Preparing conan

First you need to set __conan's remote list__ to be able to download packages prescribed in the _conanfile.py_ as requirements (dependencies). You need at least one default remote known by conan. We need at least __conancenter__ repository available. To check if it already exists run the following command:
```bash
bash> conan remote list
```
If required remote is already there you will see output alike:
```bash
bash> conan remote list
conancenter: https://center.conan.io [Verify SSL: True, Enabled: True]
```
If it doesn't appear you should install it by running the command:
```bash
bash> conan remote add conancenter https://center.conan.io
```

## Pull out and initialize

```bash
bash> git clone git@github.com:dpronin/ublk.git
```

## Configure, build and run

> :information_source: **conan** has [profiles](https://docs.conan.io/2.0/reference/config_files/profiles.html) to predefine options and environment variables in order to not provide them any time within the command line interface. To learn more about conan available actions and parameters consult `conan --help`. Also reach out to the [conan documentation](https://docs.conan.io/2/)

### Conan profile

Profile **default** used below might look like as the following:

```ini
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.libcxx=libstdc++11
compiler.version=13.2
os=Linux

[buildenv]
CXX=g++
CC=gcc
```

### Debug (with cmake presets)

```bash
bash> cd ublk
bash> conan install -s build_type=Debug -pr default --build=missing --update -of out/default .
bash> source out/default/build/Debug/generators/conanbuild.sh
bash> cmake --preset conan-debug
bash> cmake --build --preset conan-debug --parallel $(nproc)
bash> source out/default/build/Debug/generators/deactivate_conanbuild.sh
bash> source out/default/build/Debug/generators/conanrun.sh
bash> PYTHONPATH=$(pwd)/out/default/build/Debug/pyublk sudo --preserve-env=PYTHONPATH ./ublksh/ublksh.py
Type ? to list commands
ublksh>
... Working with ublksh
ublksh> Ctrl^D
bash> source out/default/build/Debug/generators/deactivate_conanrun.sh
```

### Release (with cmake presets)

```bash
bash> cd ublk
bash> conan install -s build_type=Release -pr default --build=missing --update -of out/default .
bash> source out/default/build/Release/generators/conanbuild.sh
bash> cmake --preset conan-release
bash> cmake --build --preset conan-release --parallel $(nproc)
bash> source out/default/build/Release/generators/deactivate_conanbuild.sh
bash> source out/default/build/Release/generators/conanrun.sh
bash> PYTHONPATH=$(pwd)/out/default/build/Release/pyublk sudo --preserve-env=PYTHONPATH ./ublksh/ublksh.py
Type ? to list commands
ublksh>
... Working with ublksh
ublksh> Ctrl^D
bash> source out/default/build/Release/generators/deactivate_conanrun.sh
```

## Loading ublkdrv

Before working with ublksh and configuring RAIDs and other stuff we need to load the driver to the
kernel, see [ublkdrv](https://github.com/dpronin/ublkdrv). Build the module, install and load:

```bash
bash> modprobe ublkdrv
```

## Working with ublksh

You could see examples in the directory ublksh/samples for configuring different types of RAIDs and
mirros

For example, let us create a RAID0 based on files on backend. This RAID will be 4GiB capable, with
strip 128KiB long, files on backend are f0.dat, f1.dat, f2.dat, f3.dat:

```bash
ublksh > target_create name=raid0_example capacity_sectors=8388608 type=raid0 strip_len_sectors=256 paths=f0.dat,f1.dat,f2.dat,f3.dat
ublksh > bdev_map bdev_suffix=0 target_name=raid0_example
```

Then we will see /dev/ublk-0 as a target block device that could be read from and written to
