# Project building

## Build requirements

- [cmake](https://cmake.org/) to configure the project. Minimum required version is __3.12__ unless [_cmake presets_](https://cmake.org/cmake/help/v3.19/manual/cmake-presets.7.html) feature is going to be used requiring at least __3.19__
- [conan 2.0](https://conan.io/) to download all the dependencies of the application and configure with a certain set of parameters. You can install __conan__ by giving a command to __pip__. To use __pip__ you need to install __python__ interpreter. I highly recommend to install a __python3__-based version and as the result use __pip3__ in order to avoid unexpected results with __conan__

> :information_source: In order to not accidentally harm your system python's environment you are possible to install and use [python virtual environment](https://docs.python.org/3/library/venv.html) before proceeding with conan:
> ```bash
> bash> python3 -m venv ${HOME}/.pyvenv
> bash> source ${HOME}/.pyvenv/bin/activate
> bash> pip3 install conan --upgrade
> bash> ... (further working with pip, conan, cmake, ublksh, etc.)
> bash> deactivate
> ```

In case you don't choose to use [python virtual environment](https://docs.python.org/3/library/venv.html) you can install/upgrade __conan__ within system's python environment for a current linux's user by giving the command:

```bash
$ pip3 install --user conan --upgrade
```

- A C++ compiler with at least __C++23__ support (tested __gcc >= 14__, __clang >= 18__)

## Preparing conan

First you need to set __conan's remote list__ to be able to download packages prescribed in the _conanfile.py_ as requirements (dependencies). You need at least one default remote known by conan. We need at least __conancenter__ repository available. To check if it already exists run the following command:
```bash
$ conan remote list
```
If required remote is already there you will see output alike:
```bash
$ conan remote list
conancenter: https://center.conan.io [Verify SSL: True, Enabled: True]
```
If it doesn't appear you should install it by running the command:
```bash
$ conan remote add conancenter https://center.conan.io
```

## Pull out

```bash
$ git clone git@github.com:dpronin/ublk.git
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
compiler.version=14.1
os=Linux

[buildenv]
CXX=g++-14
CC=gcc-14
```

### Debug (with cmake presets)

```bash
$ cd ublk
$ conan install -s build_type=Debug -pr default --build=missing --update -of out/default .
$ source out/default/build/Debug/generators/conanbuild.sh
$ cmake --preset conan-debug
$ cmake --build --preset conan-debug --parallel $(nproc)
$ source out/default/build/Debug/generators/deactivate_conanbuild.sh
$ source out/default/build/Debug/generators/conanrun.sh
$ sudo out/default/build/Debug/ublksh/ublksh
Version 4.3.0
Type ? to list commands
ublksh >
... Working with ublksh
ublksh > Ctrl^D
$ source out/default/build/Debug/generators/deactivate_conanrun.sh
```

### Release (with cmake presets)

```bash
$ cd ublk
$ conan install -s build_type=Release -pr default --build=missing --update -of out/default .
$ source out/default/build/Release/generators/conanbuild.sh
$ cmake --preset conan-release
$ cmake --build --preset conan-release --parallel $(nproc)
$ source out/default/build/Release/generators/deactivate_conanbuild.sh
$ source out/default/build/Release/generators/conanrun.sh
$ sudo out/default/build/Release/ublksh/ublksh
Version 4.3.0
Type ? to list commands
ublksh >
... Working with ublksh
ublksh > Ctrl^D
$ source out/default/build/Release/generators/deactivate_conanrun.sh
```

## Checking the driver out

Check it out if the kernel module required already exists:

```markdown
$ modinfo ublkdrv
filename:       /lib/modules/6.9.8-linux-x86_64/misc/ublkdrv.ko
license:        GPL
author:         Pronin Denis <dannftk@yandex.ru>
description:    UBLK driver for creating block devices that map on UBLK userspace application targets
supported:      external
version:        1.3.0
vermagic:       6.9.8-linux-x86_64 SMP preempt mod_unload
name:           ublkdrv
retpoline:      Y
depends:        uio
srcversion:     144741AC90B690082A9E3C6
```

Before working with ublksh and configuring RAIDs and other stuff we need the driver to exist, otherwise see [ublkdrv](https://github.com/dpronin/ublkdrv) how to build it.
Unless the module already exists build it up, install and rebuilt the module dependency tree to let **modprobe** find a new module.

## Working with ublksh

### Load the driver first of all

```bash
ublksh > driver_load
```

In `dmesg` we would see something like this:

```bash
> dmesg | grep ublkdrv
...
[ 1661.041485] ublkdrv: ublkdrv-1.3.0 init for kernel 6.9.8-linux-x86_64 #1 SMP PREEMPT_DYNAMIC Fri Jul  5 20:10:43 MSK 2024
...
```

### Examples of assembling block devices

You could see many examples in the [directory](ublksh/samples) for configuring different types of RAIDs, mirrors and inmem storages

#### Building RAID0 up upon extendible files on backend

This example of __RAID0__ will be based on files on backend and __4GiB__ capable, with
strip __128KiB__ long and files on backend __f0.dat__, __f1.dat__, __f2.dat__, __f3.dat__:

```bash
ublksh > target_create name=raid0_example capacity_sectors=8388608 type=raid0 strip_len_sectors=256 paths=f0.dat,f1.dat,f2.dat,f3.dat
ublksh > bdev_map bdev_suffix=0 target_name=raid0_example
```

Then we will see __/dev/ublk-0__ as a target block device:

```bash
$ lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
...
ublk-0      252:0    0     4G  0 disk
...
```

##### Check it out with performing IO operations

Let us perform IO operations to check it out.

Let us do it with dd utility performing sequential write operations thoughout all the block device:

```markdown
# dd if=/dev/random of=/dev/ublk-0 bs=1M count=4096 oflag=direct status=progress
4096+0 records in
4096+0 records out
4294967296 bytes (4.3 GB, 4.0 GiB) copied, 11.588 s, 371 MB/s
```

Let us risk to do sequential read operations by means of fio utility:

```markdown
# fio --filename=/dev/ublk-0 --direct=1 --rw=read --bs=4k --ioengine=libaio --iodepth=32 --numjobs=1 --group_reporting --name=ublk-raid0-example-read-test --eta-newline=1 --readonly
ublk-raid0-example-read-test: (g=0): rw=read, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=32
fio-3.36
Starting 1 process
Jobs: 1 (f=1): [R(1)][10.3%][r=150MiB/s][r=38.3k IOPS][eta 00m:26s]
Jobs: 1 (f=1): [R(1)][18.5%][r=157MiB/s][r=40.1k IOPS][eta 00m:22s]
Jobs: 1 (f=1): [R(1)][26.9%][r=169MiB/s][r=43.3k IOPS][eta 00m:19s]
Jobs: 1 (f=1): [R(1)][34.6%][r=167MiB/s][r=42.9k IOPS][eta 00m:17s]
Jobs: 1 (f=1): [R(1)][44.0%][r=170MiB/s][r=43.6k IOPS][eta 00m:14s]
Jobs: 1 (f=1): [R(1)][52.0%][r=173MiB/s][r=44.3k IOPS][eta 00m:12s]
Jobs: 1 (f=1): [R(1)][62.5%][r=204MiB/s][r=52.2k IOPS][eta 00m:09s]
Jobs: 1 (f=1): [R(1)][73.9%][r=208MiB/s][r=53.2k IOPS][eta 00m:06s]
Jobs: 1 (f=1): [R(1)][82.6%][r=207MiB/s][r=53.1k IOPS][eta 00m:04s]
Jobs: 1 (f=1): [R(1)][91.3%][r=141MiB/s][r=36.0k IOPS][eta 00m:02s]
Jobs: 1 (f=1): [R(1)][95.8%][r=140MiB/s][r=35.9k IOPS][eta 00m:01s]
Jobs: 1 (f=1): [R(1)][100.0%][r=138MiB/s][r=35.4k IOPS][eta 00m:00s]
ublk-raid0-example-read-test: (groupid=0, jobs=1): err= 0: pid=2887841: Fri Jun 21 19:55:18 2024
  read: IOPS=43.2k, BW=169MiB/s (177MB/s)(4096MiB/24271msec)
    slat (nsec): min=608, max=612189, avg=1488.29, stdev=1639.66
    clat (usec): min=25, max=17904, avg=738.84, stdev=561.81
     lat (usec): min=26, max=17906, avg=740.33, stdev=561.84
    clat percentiles (usec):
     |  1.00th=[   65],  5.00th=[  116], 10.00th=[  169], 20.00th=[  269],
     | 30.00th=[  379], 40.00th=[  506], 50.00th=[  635], 60.00th=[  775],
     | 70.00th=[  914], 80.00th=[ 1074], 90.00th=[ 1450], 95.00th=[ 1844],
     | 99.00th=[ 2507], 99.50th=[ 2802], 99.90th=[ 3621], 99.95th=[ 4293],
     | 99.99th=[ 9503]
   bw (  KiB/s): min=134032, max=217752, per=100.00%, avg=173247.33, stdev=25781.42, samples=48
   iops        : min=33508, max=54438, avg=43311.83, stdev=6445.35, samples=48
  lat (usec)   : 50=0.01%, 100=3.64%, 250=14.48%, 500=21.49%, 750=18.54%
  lat (usec)   : 1000=17.81%
  lat (msec)   : 2=20.44%, 4=3.53%, 10=0.06%, 20=0.01%
  cpu          : usr=4.91%, sys=11.77%, ctx=1020589, majf=0, minf=45
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=100.0%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.1%, 64=0.0%, >=64=0.0%
     issued rwts: total=1048576,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=32

Run status group 0 (all jobs):
   READ: bw=169MiB/s (177MB/s), 169MiB/s-169MiB/s (177MB/s-177MB/s), io=4096MiB (4295MB), run=24271-24271msec

Disk stats (read/write):
  ublk-0: ios=1043698/0, sectors=8349584/0, merge=0/0, ticks=768038/0, in_queue=768038, util=99.62%
```

##### Removing RAID0 device

To unload __/dev/ublk-0__ device perform unmapping the block device from the ublk's target:

```bash
ublksh > bdev_unmap bdev_suffix=0
```

Then you may destroy the target by giving its name:

```bash
ublksh > target_destroy name=raid0_example
```

#### Building RAID1 up upon nullb devices

##### Checking and loading _null_blk_ kernel module

Make sure that you have _null_blk_ module built in the kernel or existing in the out-of-the-box list of modules.

At first, check your kernel config out if it has been built in the kernel image by accessing to proc's file system node:

```bash
$ zcat /proc/config.gz| grep -i null_b
CONFIG_BLK_DEV_NULL_BLK=m
```

Then, check if your system has knowledge how to load _null_blk_ kernel module:

```bash
$ modinfo null_blk
filename:       /lib/modules/6.9.8-linux-x86_64/kernel/drivers/block/null_blk/null_blk.ko
author:         Jens Axboe <axboe@kernel.dk>
license:        GPL
vermagic:       6.9.8-linux-x86_64 SMP preempt mod_unload
name:           null_blk
intree:         Y
retpoline:      Y
depends:        configfs
parm:           zone_max_active:Maximum number of active zones when block device is zoned. Default: 0 (no limit) (uint)
parm:           zone_max_open:Maximum number of open zones when block device is zoned. Default: 0 (no limit) (uint)
parm:           zone_nr_conv:Number of conventional zones when block device is zoned. Default: 0 (uint)
parm:           zone_capacity:Zone capacity in MB when block device is zoned. Can be less than or equal to zone size. Default: Zone size (ulong)
parm:           zone_size:Zone size in MB when block device is zoned. Must be power-of-two: Default: 256 (ulong)
parm:           zoned:Make device as a host-managed zoned block device. Default: false (bool)
parm:           mbps:Limit maximum bandwidth (in MiB/s). Default: 0 (no limit) (uint)
parm:           cache_size:ulong
parm:           discard:Support discard operations (requires memory-backed null_blk device). Default: false (bool)
parm:           memory_backed:Create a memory-backed block device. Default: false (bool)
parm:           use_per_node_hctx:Use per-node allocation for hardware context queues. Default: false (bool)
parm:           hw_queue_depth:Queue depth for each hardware queue. Default: 64 (int)
parm:           completion_nsec:Time in ns to complete a request in hardware. Default: 10,000ns (ulong)
parm:           irqmode:IRQ completion handler. 0-none, 1-softirq, 2-timer
parm:           shared_tag_bitmap:Use shared tag bitmap for all submission queues for blk-mq (bool)
parm:           shared_tags:Share tag set between devices for blk-mq (bool)
parm:           blocking:Register as a blocking blk-mq driver device (bool)
parm:           nr_devices:Number of devices to register (uint)
parm:           max_sectors:Maximum size of a command (in 512B sectors) (int)
parm:           bs:Block size (in bytes) (int)
parm:           gb:Size in GB (int)
parm:           queue_mode:Block interface to use (0=bio,1=rq,2=multiqueue)
parm:           home_node:Home node for the device (int)
parm:           poll_queues:Number of IOPOLL submission queues (int)
parm:           submit_queues:Number of submission queues (int)
parm:           no_sched:No io scheduler (int)
parm:           virt_boundary:Require a virtual boundary for the device. Default: False (bool)
```

Then, insert _null_blk_ module with specific list of parameters before proceeding with building
RAID1:

```markdown
# modprobe null_blk nr_devices=6 gb=6
```

> :information_source: parameters given for null_blk module could be changed by a user, they may not have exactly the same values as given above. Depending on what a user wants and how they want to build a RAID upon _nullb\*_ devices parameters could vary

##### Building RAID1 up

This example of __RAID1__ will be based on _nullb\*_ devices on backend, the RAID is going to be __6GiB__ capable, devices on backend will be __/dev/nullb0__, __/dev/nullb1__, __/dev/nullb2__, __/dev/nullb3__, __/dev/nullb4__, __/dev/nullb5__:

```bash
ublksh > target_create name=raid1_example capacity_sectors=12582912 type=raid1 paths=/dev/nullb0,/dev/nullb1,/dev/nullb2,/dev/nullb3,/dev/nullb4,/dev/nullb5
ublksh > bdev_map bdev_suffix=0 target_name=raid1_example
```

Then we will see __/dev/ublk-0__ as a target block device:

```bash
$ lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
...
ublk-0      252:0    0     6G  0 disk
...
```

##### Check it out with performing IO operations

Let us perform IO operations to check it out.

Let us do it with dd utility performing sequential write operations thoughout all the block device:

```markdown
# dd if=/dev/random of=/dev/ublk-0 oflag=direct bs=128K count=49152 status=progress
49152+0 records in
49152+0 records out
6442450944 bytes (6.4 GB, 6.0 GiB) copied, 18.1631 s, 355 MB/s
```

While _dd_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           0,75    0,00   12,17    2,89    0,00   84,19

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
nullb0         5566,00         0,00       347,88         0,00          0        347          0
nullb1         5566,00         0,00       347,88         0,00          0        347          0
nullb2         5566,00         0,00       347,88         0,00          0        347          0
nullb3         5566,00         0,00       347,88         0,00          0        347          0
nullb4         5566,00         0,00       347,88         0,00          0        347          0
nullb5         5566,00         0,00       347,88         0,00          0        347          0
ublk-0         2783,00         0,00       347,88         0,00          0        347          0
...
```

As we see, RAID1, that is built upon 6 null-block devices under the hood, mirrors write IO operations to each device on backend

Let us do sequential read operations by means of fio utility and see how it would go:

```markdown
# fio --filename=/dev/ublk-0 --direct=1 --rw=read --bs=128k --io_size=100000m --ioengine=libaio --iodepth=32 --numjobs=1 --group_reporting --name=ublk-raid1-example-read-test --eta-newline=1 --readonly
ublk-raid1-example-read-test: (g=0): rw=read, bs=(R) 128KiB-128KiB, (W) 128KiB-128KiB, (T) 128KiB-128KiB, ioengine=libaio, iodepth=32
fio-3.36
Starting 1 process
Jobs: 1 (f=1): [R(1)][30.0%][r=9715MiB/s][r=77.7k IOPS][eta 00m:07s]
Jobs: 1 (f=1): [R(1)][50.0%][r=9613MiB/s][r=76.9k IOPS][eta 00m:05s]
Jobs: 1 (f=1): [R(1)][70.0%][r=9596MiB/s][r=76.8k IOPS][eta 00m:03s]
Jobs: 1 (f=1): [R(1)][90.0%][r=9668MiB/s][r=77.3k IOPS][eta 00m:01s]
Jobs: 1 (f=1): [R(1)][100.0%][r=9845MiB/s][r=78.8k IOPS][eta 00m:00s]
ublk-raid1-example-read-test: (groupid=0, jobs=1): err= 0: pid=2889276: Fri Jun 21 19:58:58 2024
  read: IOPS=77.4k, BW=9675MiB/s (10.1GB/s)(97.7GiB/10336msec)
    slat (nsec): min=1844, max=1050.8k, avg=4485.88, stdev=2862.77
    clat (usec): min=200, max=5917, avg=408.47, stdev=87.77
     lat (usec): min=203, max=5925, avg=412.96, stdev=88.23
    clat percentiles (usec):
     |  1.00th=[  306],  5.00th=[  330], 10.00th=[  343], 20.00th=[  359],
     | 30.00th=[  371], 40.00th=[  383], 50.00th=[  396], 60.00th=[  408],
     | 70.00th=[  424], 80.00th=[  445], 90.00th=[  478], 95.00th=[  515],
     | 99.00th=[  701], 99.50th=[  766], 99.90th=[  963], 99.95th=[ 1123],
     | 99.99th=[ 3752]
   bw (  MiB/s): min= 9254, max=10054, per=100.00%, avg=9693.24, stdev=170.57, samples=20
   iops        : min=74038, max=80432, avg=77545.90, stdev=1364.54, samples=20
  lat (usec)   : 250=0.01%, 500=93.69%, 750=5.72%, 1000=0.51%
  lat (msec)   : 2=0.06%, 4=0.01%, 10=0.01%
  cpu          : usr=7.55%, sys=39.79%, ctx=323889, majf=0, minf=1038
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=99.9%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.1%, 64=0.0%, >=64=0.0%
     issued rwts: total=800000,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=32

Run status group 0 (all jobs):
   READ: bw=9675MiB/s (10.1GB/s), 9675MiB/s-9675MiB/s (10.1GB/s-10.1GB/s), io=97.7GiB (105GB), run=10336-10336msec

Disk stats (read/write):
  ublk-0: ios=786816/0, sectors=201424896/0, merge=0/0, ticks=316846/0, in_queue=316846, util=99.03%
```

While _fio_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           3,83    0,00   27,48    0,00    0,00   68,69

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
nullb0        26166,00      1635,38         0,00         0,00       1635          0          0
nullb1        26166,00      1635,38         0,00         0,00       1635          0          0
nullb2        26166,00      1635,38         0,00         0,00       1635          0          0
nullb3        26166,00      1635,38         0,00         0,00       1635          0          0
nullb4        26166,00      1635,38         0,00         0,00       1635          0          0
nullb5        26165,00      1635,31         0,00         0,00       1635          0          0
ublk-0        78500,00      9812,50         0,00         0,00       9812          0          0
...
```

As we see, __RAID1__ benefits from uniformly distributing read IO operations among all the devices on backend

##### Removing RAID1 device

To unload __/dev/ublk-0__ device perform unmapping the block device from the ublk's target:

```bash
ublksh > bdev_unmap bdev_suffix=0
```

Then you may destroy the target by giving its name:

```bash
ublksh > target_destroy name=raid1_example
```

##### Removing _nullb\*_ devices

If you finish working with _nullb\*_ devices you may remove the module from kernel:

```markdown
# rmmod null_blk
```

#### Building RAID5 up upon loop devices and use RAID built for deploying ext4 file system upon it

##### Checking and loading loop kernel module

Make sure that you have _loop_ module built in the kernel or existing in the out-of-the-box list of modules.

At first, check your kernel config out if it has been built in the kernel image by accessing to proc's file system node:

```bash
$ zcat /proc/config.gz| grep -i loop
CONFIG_BLK_DEV_LOOP=m
CONFIG_BLK_DEV_LOOP_MIN_COUNT=8
...
```

Then, check if your system has knowledge how to load _loop_ kernel module:

```bash
$ modinfo loop
filename:       /lib/modules/6.9.8-linux-x86_64/kernel/drivers/block/loop.ko
license:        GPL
alias:          block-major-7-*
alias:          char-major-10-237
alias:          devname:loop-control
vermagic:       6.9.8-linux-x86_64 SMP preempt mod_unload
name:           loop
intree:         Y
retpoline:      Y
depends:
parm:           hw_queue_depth:Queue depth for each hardware queue. Default: 128
parm:           max_part:Maximum number of partitions per loop device (int)
parm:           max_loop:Maximum number of loop devices
```

Then, insert the _loop_ module with specific list of parameters before proceeding with building
RAID5:

```markdown
# modprobe loop
```

##### Preparing loop devices

__RAID5__ requires __at least 3 devices__ on backend, let us prepare 3 loop devices mapped to regular files 200MiB capable apiece:

At first, prepare 3 files with fixed required size:

```bash
$ for i in 0 1 2; do dd if=/dev/zero of=$i.dat bs=1M count=200 oflag=direct; done
200+0 records in
200+0 records out
209715200 bytes (210 MB, 200 MiB) copied, 0.118051 s, 1.8 GB/s
200+0 records in
200+0 records out
209715200 bytes (210 MB, 200 MiB) copied, 0.111347 s, 1.9 GB/s
200+0 records in
200+0 records out
209715200 bytes (210 MB, 200 MiB) copied, 0.107954 s, 1.9 GB/s
$ ls {0..2}.dat
0.dat  1.dat  2.dat
```

Then, we setup loop devices, each being mapped to its own file created above:

```markdown
# for i in 0 1 2; do losetup /dev/loop$i $i.dat; done
```

List block devices to ensure loop devices exist:

```bash
$ lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
loop0         7:0    0   200M  0 loop
loop1         7:1    0   200M  0 loop
loop2         7:2    0   200M  0 loop
...
```

Ok. Now we are ready to build __RAID5__ on these loop devices

##### Building RAID5 up

This example of __RAID5__ will be based on _loop_-based devices on backend, the RAID is going to be __400MiB__ capable, with __32KiB__ strip long, devices on backend will be __/dev/loop0__, __/dev/loop1__, __/dev/loop2__:

```bash
ublksh > target_create name=raid5_example capacity_sectors=819200 type=raid5 strip_len_sectors=64 paths=/dev/loop0,/dev/loop1,/dev/loop2
ublksh > bdev_map bdev_suffix=0 target_name=raid5_example
```

Then we will see __/dev/ublk-0__ as a target block device:

```bash
$ lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
...
ublk-0      252:0    0   400M  0 disk
...
```

##### Deploying ext4 file system upon RAID5 just built

Run making file system on block device representing our RAID5 assembled above:

```markdown
# mkfs.ext4 /dev/ublk-0
mke2fs 1.47.0 (5-Feb-2023)
Creating filesystem with 409600 1k blocks and 102400 inodes
Filesystem UUID: 9c6e1318-89c4-47fc-bbf5-dcd2870ec854
Superblock backups stored on blocks:
  8193, 24577, 40961, 57345, 73729, 204801, 221185, 401409

Allocating group tables: done
Writing inode tables: done
Creating journal (8192 blocks): done
Writing superblocks and filesystem accounting information: done
```

Then, we need to mount the file system to a new mountpoint:

```markdown
# mkdir -p raid5ext4mp
# mount /dev/ublk-0 raid5ext4mp
# mount | grep raid5ext4mp
/dev/ublk-0 on .../raid5ext4mp type ext4 (rw,relatime)
# df -h | grep -i /dev/ublk-0
/dev/ublk-0        365M          14K  341M            1% .../raid5ext4mp
```

We see __365MiB__ capable a new file system mounted to our mountpoint

##### Check it out with performing IO operations

Let us perform IO operations to check it out.

For the beginning we try to use dd utility performing sequential write operations to a file from the file system we have built upon __RAID5__. We're going to write 200MiB of data, each block being 4KiB long at a time of write IO:

```markdown
# dd if=/dev/random of=raid5ext4mp/a.dat oflag=direct bs=4K count=51200 status=progress
51200+0 records in
51200+0 records out
209715200 bytes (210 MB, 200 MiB) copied, 4.08921 s, 51.3 MB/s
```

If we take a look at _iostat_'s measurements while _dd_ is working we will see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           5,54    0,00   14,69    8,76    0,00   71,01

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
loop0         18369,00       166,14       158,41         0,00        166        158          0
loop1         18596,00       165,67       157,56         0,00        165        157          0
loop2         18880,00       166,05       157,73         0,00        166        157          0
ublk-0        12764,00         0,00        49,86         0,00          0         49          0
...
```

Let us do IO write operations by means of fio utility and see how it would go. The most important thing here is that we want to check if data corruption takes place, therefore fio is going to be configured with data verification option. File size will be constrained to 100MiB for this test, write operations will be randomly distributed within these 100MiB space, see:

```markdown
# fio --filename=raid5ext4mp/b.dat --filesize=100M --direct=1 --rw=randrw --bs=16K --ioengine=libaio --iodepth=8 --numjobs=1 --group_reporting --name=ublk-raid5ext4-example-write-verify-test --eta-newline=1 --verify=xxhash --loops=100
ublk-raid5ext4-example-write-verify-test: (g=0): rw=randrw, bs=(R) 16.0KiB-16.0KiB, (W) 16.0KiB-16.0KiB, (T) 16.0KiB-16.0KiB, ioengine=libaio, iodepth=8
fio-3.36
Starting 1 process
Jobs: 1 (f=1): [V(1)][13.0%][r=435MiB/s,w=231MiB/s][r=27.8k,w=14.8k IOPS][eta 00m:20s]
Jobs: 1 (f=1): [m(1)][22.7%][r=428MiB/s,w=234MiB/s][r=27.4k,w=15.0k IOPS][eta 00m:17s]
Jobs: 1 (f=1): [m(1)][31.8%][r=426MiB/s,w=232MiB/s][r=27.3k,w=14.8k IOPS][eta 00m:15s]
Jobs: 1 (f=1): [m(1)][40.9%][r=428MiB/s,w=233MiB/s][r=27.4k,w=14.9k IOPS][eta 00m:13s]
Jobs: 1 (f=1): [m(1)][50.0%][r=427MiB/s,w=232MiB/s][r=27.3k,w=14.9k IOPS][eta 00m:11s]
Jobs: 1 (f=1): [m(1)][59.1%][r=432MiB/s,w=233MiB/s][r=27.6k,w=14.9k IOPS][eta 00m:09s]
Jobs: 1 (f=1): [m(1)][68.2%][r=440MiB/s,w=230MiB/s][r=28.2k,w=14.7k IOPS][eta 00m:07s]
Jobs: 1 (f=1): [V(1)][77.3%][r=425MiB/s,w=204MiB/s][r=27.2k,w=13.1k IOPS][eta 00m:05s]
Jobs: 1 (f=1): [m(1)][86.4%][r=409MiB/s,w=214MiB/s][r=26.2k,w=13.7k IOPS][eta 00m:03s]
Jobs: 1 (f=1): [m(1)][95.5%][r=465MiB/s,w=219MiB/s][r=29.7k,w=14.0k IOPS][eta 00m:01s]
Jobs: 1 (f=1): [m(1)][100.0%][r=428MiB/s,w=234MiB/s][r=27.4k,w=15.0k IOPS][eta 00m:00s]
ublk-raid5ext4-example-write-verify-test: (groupid=0, jobs=1): err= 0: pid=2893951: Fri Jun 21 20:11:05 2024
  read: IOPS=28.2k, BW=441MiB/s (462MB/s)(9.77GiB/22680msec)
    slat (nsec): min=953, max=623046, avg=2515.69, stdev=1961.09
    clat (usec): min=23, max=6959, avg=119.65, stdev=59.01
     lat (usec): min=26, max=6960, avg=122.16, stdev=59.29
    clat percentiles (usec):
     |  1.00th=[   39],  5.00th=[   50], 10.00th=[   59], 20.00th=[   75],
     | 30.00th=[   90], 40.00th=[  103], 50.00th=[  116], 60.00th=[  127],
     | 70.00th=[  139], 80.00th=[  155], 90.00th=[  180], 95.00th=[  204],
     | 99.00th=[  302], 99.50th=[  383], 99.90th=[  490], 99.95th=[  537],
     | 99.99th=[ 1057]
   bw (  KiB/s): min=196480, max=234848, per=48.94%, avg=220945.07, stdev=10757.71, samples=45
   iops        : min=12280, max=14678, avg=13809.07, stdev=672.36, samples=45
  write: IOPS=18.3k, BW=285MiB/s (299MB/s)(5109MiB/17906msec); 0 zone resets
    slat (usec): min=4, max=653, avg= 8.96, stdev= 5.12
    clat (usec): min=76, max=7218, avg=300.55, stdev=102.22
     lat (usec): min=84, max=7227, avg=309.51, stdev=102.87
    clat percentiles (usec):
     |  1.00th=[  135],  5.00th=[  180], 10.00th=[  206], 20.00th=[  235],
     | 30.00th=[  255], 40.00th=[  273], 50.00th=[  289], 60.00th=[  306],
     | 70.00th=[  326], 80.00th=[  355], 90.00th=[  396], 95.00th=[  453],
     | 99.00th=[  619], 99.50th=[  668], 99.90th=[  840], 99.95th=[ 1074],
     | 99.99th=[ 3458]
   bw (  KiB/s): min=205376, max=245504, per=79.00%, avg=230833.07, stdev=11138.42, samples=45
   iops        : min=12836, max=15344, avg=14427.02, stdev=696.13, samples=45
  lat (usec)   : 50=3.43%, 100=21.57%, 250=49.15%, 500=24.67%, 750=1.11%
  lat (usec)   : 1000=0.05%
  lat (msec)   : 2=0.02%, 4=0.01%, 10=0.01%
  cpu          : usr=21.16%, sys=14.94%, ctx=798247, majf=0, minf=106
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=99.9%, 16=0.0%, 32=0.0%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.1%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     issued rwts: total=640000,327000,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=8

Run status group 0 (all jobs):
   READ: bw=441MiB/s (462MB/s), 441MiB/s-441MiB/s (462MB/s-462MB/s), io=9.77GiB (10.5GB), run=22680-22680msec
  WRITE: bw=285MiB/s (299MB/s), 285MiB/s-285MiB/s (299MB/s-299MB/s), io=5109MiB (5358MB), run=17906-17906msec

Disk stats (read/write):
  ublk-0: ios=637517/327012, sectors=20400544/10464024, merge=0/0, ticks=74299/97105, in_queue=171404, util=99.55%
```

While _fio_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle                                                                                                                                           20:11:00 [50/1868]
          10,16    0,00   28,78    0,00    0,00   61,07

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
loop0         41057,00       416,28       271,79         0,00        416        271          0
loop1         41015,00       411,62       266,91         0,00        411        266          0
loop2         40966,00       414,42       269,91         0,00        414        269          0
ublk-0        42598,00       433,61       231,98         0,00        433        231          0
...
```

##### Cleaning everything created earlier up

First of all, you need to unmount the file system from mountpoint _raid5ext4mp_:

```markdown
# umount raid5ext4mp
```

Then, to unload __/dev/ublk-0__ device perform unmapping the block device from the ublk's target:

```bash
ublksh > bdev_unmap bdev_suffix=0
```

Then you may destroy the target by giving its name:

```bash
ublksh > target_destroy name=raid5_example
```

Then, you may detach loop devices from the files backing them:

```markdown
# for i in 0 1 2; do losetup -d /dev/loop$i; done
```

Then, if you need, backing files __0.dat__, __1.dat__ and __2.dat__ may be removed

> :information_source: If you want to recover everything up and again see files generated by IO tests done above you could build the same RAID5 with the same configuration of RAID itself and loop devices from the start, then mount already existing file system again (skip making file system phase, otherwise you will wipe everything off) and see that nothing has been broken and file system has stayed consistent and contained the files __a.dat__ and __b.dat__

Finally, unload __loop__ devices driver if required:

```markdown
# rmmod loop
```

## Install

### Debug configuration

```bash
$ cmake --install out/default/build/Debug --config Debug
```

### Release configuration

```bash
$ cmake --install out/default/build/Release --config Release
```
