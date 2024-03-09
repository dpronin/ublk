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

## Pull out

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

You could see many examples in the [directory](ublksh/samples) for configuring different types of RAIDs, mirros and inmem storages

### Example of building RAID0

An example of RAID0 will be based on files on backend. This RAID will be __4GiB__ capable, with
strip __128KiB__ long, files on backend are __f0.dat__, __f1.dat__, __f2.dat__, __f3.dat__:

```bash
ublksh > target_create name=raid0_example capacity_sectors=8388608 type=raid0 strip_len_sectors=256 paths=f0.dat,f1.dat,f2.dat,f3.dat
ublksh > bdev_map bdev_suffix=0 target_name=raid0_example
```

Then we will see __/dev/ublk-0__ as a target block device:

```bash
bash > lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
ublk-0      252:0    0     4G  0 disk
...
```

Let's perform IO operations to check it out.

Let us do it with dd utility performing sequential write operations thoughout all the block device:

```bash
bash > dd if=/dev/random of=/dev/ublk-0 bs=1M count=4096 oflag=direct status=progress
4191158272 bytes (4.2 GB, 3.9 GiB) copied, 14 s, 299 MB/s
4096+0 records in
4096+0 records out
4294967296 bytes (4.3 GB, 4.0 GiB) copied, 14.3623 s, 299 MB/s
```

Let's us risk to do sequential read operations by means of fio utility:

```bash
bash > fio --filename=/dev/ublk-0 --direct=1 --rw=read --bs=4k --ioengine=libaio --iodepth=32 --numjobs=1 --group_reporting --name=ublk-raid0-example-read-test --eta-newline=1 --readonly
ublk-raid0-example-read-test: (g=0): rw=read, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=32
fio-3.34
Starting 1 process
Jobs: 1 (f=1): [R(1)][7.7%][r=105MiB/s][r=27.0k IOPS][eta 00m:36s]
Jobs: 1 (f=1): [R(1)][13.2%][r=109MiB/s][r=27.8k IOPS][eta 00m:33s]
Jobs: 1 (f=1): [R(1)][18.4%][r=108MiB/s][r=27.7k IOPS][eta 00m:31s]
Jobs: 1 (f=1): [R(1)][23.7%][r=106MiB/s][r=27.1k IOPS][eta 00m:29s]
Jobs: 1 (f=1): [R(1)][28.9%][r=111MiB/s][r=28.5k IOPS][eta 00m:27s]
Jobs: 1 (f=1): [R(1)][35.1%][r=125MiB/s][r=32.1k IOPS][eta 00m:24s]
Jobs: 1 (f=1): [R(1)][40.5%][r=109MiB/s][r=27.9k IOPS][eta 00m:22s]
Jobs: 1 (f=1): [R(1)][47.2%][r=129MiB/s][r=33.1k IOPS][eta 00m:19s]
Jobs: 1 (f=1): [R(1)][52.8%][r=123MiB/s][r=31.6k IOPS][eta 00m:17s]
Jobs: 1 (f=1): [R(1)][58.3%][r=95.2MiB/s][r=24.4k IOPS][eta 00m:15s]
Jobs: 1 (f=1): [R(1)][63.9%][r=103MiB/s][r=26.3k IOPS][eta 00m:13s]
Jobs: 1 (f=1): [R(1)][69.4%][r=112MiB/s][r=28.7k IOPS][eta 00m:11s]
Jobs: 1 (f=1): [R(1)][75.0%][r=110MiB/s][r=28.3k IOPS][eta 00m:09s]
Jobs: 1 (f=1): [R(1)][80.6%][r=123MiB/s][r=31.6k IOPS][eta 00m:07s]
Jobs: 1 (f=1): [R(1)][86.1%][r=129MiB/s][r=33.1k IOPS][eta 00m:05s]
Jobs: 1 (f=1): [R(1)][94.3%][r=129MiB/s][r=33.1k IOPS][eta 00m:02s]
Jobs: 1 (f=1): [R(1)][100.0%][r=128MiB/s][r=32.8k IOPS][eta 00m:00s]
ublk-raid0-example-read-test: (groupid=0, jobs=1): err= 0: pid=327122: Sat Mar  9 13:35:45 2024
  read: IOPS=29.5k, BW=115MiB/s (121MB/s)(4096MiB/35520msec)
    slat (nsec): min=887, max=745083, avg=2308.27, stdev=1793.58
    clat (usec): min=64, max=19475, avg=1081.17, stdev=668.80
     lat (usec): min=67, max=19478, avg=1083.48, stdev=668.89
    clat percentiles (usec):
     |  1.00th=[  116],  5.00th=[  192], 10.00th=[  293], 20.00th=[  490],
     | 30.00th=[  668], 40.00th=[  832], 50.00th=[  996], 60.00th=[ 1188],
     | 70.00th=[ 1385], 80.00th=[ 1598], 90.00th=[ 1909], 95.00th=[ 2245],
     | 99.00th=[ 2900], 99.50th=[ 3163], 99.90th=[ 3949], 99.95th=[ 4948],
     | 99.99th=[12387]
   bw (  KiB/s): min=92384, max=137408, per=100.00%, avg=118138.48, stdev=11073.42, samples=71
   iops        : min=23096, max=34352, avg=29534.62, stdev=2768.37, samples=71
  lat (usec)   : 100=0.22%, 250=7.62%, 500=12.79%, 750=14.35%, 1000=15.21%
  lat (msec)   : 2=41.62%, 4=8.09%, 10=0.08%, 20=0.02%
  cpu          : usr=4.62%, sys=11.43%, ctx=985667, majf=0, minf=41
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=100.0%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.1%, 64=0.0%, >=64=0.0%
     issued rwts: total=1048576,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=32

Run status group 0 (all jobs):
   READ: bw=115MiB/s (121MB/s), 115MiB/s-115MiB/s (121MB/s-121MB/s), io=4096MiB (4295MB), run=35520-35520msec

Disk stats (read/write):
  ublk-0: ios=1045025/0, merge=0/0, ticks=1126357/0, in_queue=1126357, util=99.76%
```

To unload __/dev/ublk-0__ device perform unmapping the block device from the ublk's target:

```bash
ublksh > bdev_unmap bdev_suffix=0
```

Then you may destroy the target by giving its name:

```bash
ublksh > target_destroy name=raid0_example
```
