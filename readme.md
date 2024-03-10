# Project building

## Build requirements

- [cmake](https://cmake.org/) to configure the project. Minimum required version is __3.19__ if [_cmake presets_](https://cmake.org/cmake/help/v3.19/manual/cmake-presets.7.html) are going to be used, otherwise __3.12__ would do
- [conan 2.0](https://conan.io/) to download all the dependencies of the application and configure with a certain set of parameters. You can install __conan__ by giving a command to __pip__. To use __pip__ you need to install __python__ interpreter. I highly recommend to install a __python3__-based version and as the result use __pip3__ in order to avoid unexpected results with __conan__

To install/upgrade __conan__ within system's python environment for a current linux's user give the command:

```bash
$ pip3 install --user conan --upgrade
```

- A C++ compiler with at least __C++23__ support

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
compiler.version=13.2
os=Linux

[buildenv]
CXX=g++
CC=gcc
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
$ PYTHONPATH=$(pwd)/out/default/build/Debug/pyublk sudo --preserve-env=PYTHONPATH ./ublksh/ublksh.py
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
$ PYTHONPATH=$(pwd)/out/default/build/Release/pyublk sudo --preserve-env=PYTHONPATH ./ublksh/ublksh.py
Type ? to list commands
ublksh >
... Working with ublksh
ublksh > Ctrl^D
$ source out/default/build/Release/generators/deactivate_conanrun.sh
```

## Loading ublkdrv

Before working with ublksh and configuring RAIDs and other stuff we need to load the driver to the
kernel, see [ublkdrv](https://github.com/dpronin/ublkdrv). Build the module, install and load:

```markdown
# modprobe ublkdrv
```

## Working with ublksh

You could see many examples in the [directory](ublksh/samples) for configuring different types of RAIDs, mirrors and inmem storages

### Examples of assembling block devices

#### Building RAID0 upon extendible files on backend

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
4191158272 bytes (4.2 GB, 3.9 GiB) copied, 14 s, 299 MB/s
4096+0 records in
4096+0 records out
4294967296 bytes (4.3 GB, 4.0 GiB) copied, 14.3623 s, 299 MB/s
```

Let us us risk to do sequential read operations by means of fio utility:

```markdown
# fio --filename=/dev/ublk-0 --direct=1 --rw=read --bs=4k --ioengine=libaio --iodepth=32 --numjobs=1 --group_reporting --name=ublk-raid0-example-read-test --eta-newline=1 --readonly
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

##### Removing RAID0 device

To unload __/dev/ublk-0__ device perform unmapping the block device from the ublk's target:

```bash
ublksh > bdev_unmap bdev_suffix=0
```

Then you may destroy the target by giving its name:

```bash
ublksh > target_destroy name=raid0_example
```

#### Building RAID1 upon nullb devices

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
filename:       /lib/modules/6.7.5/kernel/drivers/block/null_blk/null_blk.ko
author:         Jens Axboe <axboe@kernel.dk>
license:        GPL
vermagic:       6.7.5 SMP preempt mod_unload
name:           null_blk
intree:         Y
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

Then, insert the _null_blk_ module with specific list of parameters before proceeding with building
RAID1:

```markdown
# modprobe null_blk nr_devices=6 gb=6
```

> :information_source: parameters given for null_blk module could be changed by a user, they may not have exactly the same values as given above. Depending on what a user wants and how they want to build a RAID upon _nullb\*_ devices parameters could vary

##### Building RAID1

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
6304301056 bytes (6.3 GB, 5.9 GiB) copied, 26 s, 242 MB/s
49152+0 records in
49152+0 records out
6442450944 bytes (6.4 GB, 6.0 GiB) copied, 26.5802 s, 242 MB/s
```

While _dd_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           3,64    0,00   12,69    3,27    0,00   80,40

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
nullb0         3808,00         0,00       238,00         0,00          0        238          0
nullb1         3808,00         0,00       238,00         0,00          0        238          0
nullb2         3808,00         0,00       238,00         0,00          0        238          0
nullb3         3808,00         0,00       238,00         0,00          0        238          0
nullb4         3808,00         0,00       238,00         0,00          0        238          0
nullb5         3808,00         0,00       238,00         0,00          0        238          0
ublk-0         1904,00         0,00       238,00         0,00          0        238          0

...
```

As we see, RAID1, that is built upon 6 null-block devices under the hood, mirrors write IO operations to each device on backend

Let us do sequential read operations by means of fio utility and see how it would go:

```markdown
# fio --filename=/dev/ublk-0 --direct=1 --rw=read --bs=128k --io_size=100000m --ioengine=libaio --iodepth=32 --numjobs=1 --group_reporting --name=ublk-raid1-example-read-test --eta-newline=1 --readonly
ublk-raid1-example-read-test: (g=0): rw=read, bs=(R) 128KiB-128KiB, (W) 128KiB-128KiB, (T) 128KiB-128KiB, ioengine=libaio, iodepth=32
fio-3.34
Starting 1 process
Jobs: 1 (f=1): [R(1)][12.5%][r=4140MiB/s][r=33.1k IOPS][eta 00m:21s]
Jobs: 1 (f=1): [R(1)][20.0%][r=3719MiB/s][r=29.8k IOPS][eta 00m:20s]
Jobs: 1 (f=1): [R(1)][28.0%][r=3652MiB/s][r=29.2k IOPS][eta 00m:18s]
Jobs: 1 (f=1): [R(1)][34.6%][r=3735MiB/s][r=29.9k IOPS][eta 00m:17s]
Jobs: 1 (f=1): [R(1)][44.0%][r=4183MiB/s][r=33.5k IOPS][eta 00m:14s]
Jobs: 1 (f=1): [R(1)][52.0%][r=3959MiB/s][r=31.7k IOPS][eta 00m:12s]
Jobs: 1 (f=1): [R(1)][60.0%][r=4086MiB/s][r=32.7k IOPS][eta 00m:10s]
Jobs: 1 (f=1): [R(1)][68.0%][r=4035MiB/s][r=32.3k IOPS][eta 00m:08s]
Jobs: 1 (f=1): [R(1)][76.0%][r=3690MiB/s][r=29.5k IOPS][eta 00m:06s]
Jobs: 1 (f=1): [R(1)][84.0%][r=4150MiB/s][r=33.2k IOPS][eta 00m:04s]
Jobs: 1 (f=1): [R(1)][92.0%][r=4143MiB/s][r=33.1k IOPS][eta 00m:02s]
Jobs: 1 (f=1): [R(1)][100.0%][r=3908MiB/s][r=31.3k IOPS][eta 00m:00s]
ublk-raid1-example-read-test: (groupid=0, jobs=1): err= 0: pid=14939: Sun Mar 10 14:44:35 2024
  read: IOPS=31.5k, BW=3943MiB/s (4135MB/s)(97.7GiB/25360msec)
    slat (usec): min=3, max=775, avg= 7.71, stdev= 4.15
    clat (usec): min=214, max=7832, avg=1005.97, stdev=207.37
     lat (usec): min=222, max=7839, avg=1013.68, stdev=208.41
    clat percentiles (usec):
     |  1.00th=[  742],  5.00th=[  791], 10.00th=[  816], 20.00th=[  857],
     | 30.00th=[  889], 40.00th=[  922], 50.00th=[  955], 60.00th=[  996],
     | 70.00th=[ 1045], 80.00th=[ 1123], 90.00th=[ 1270], 95.00th=[ 1385],
     | 99.00th=[ 1663], 99.50th=[ 1778], 99.90th=[ 2073], 99.95th=[ 2311],
     | 99.99th=[ 6194]
   bw (  MiB/s): min= 3425, max= 4219, per=100.00%, avg=3948.34, stdev=234.75, samples=50
   iops        : min=27402, max=33756, avg=31586.76, stdev=1878.00, samples=50
  lat (usec)   : 250=0.01%, 500=0.01%, 750=1.41%, 1000=59.39%
  lat (msec)   : 2=39.05%, 4=0.12%, 10=0.02%
  cpu          : usr=4.68%, sys=27.71%, ctx=325744, majf=0, minf=1035
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=99.9%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.1%, 64=0.0%, >=64=0.0%
     issued rwts: total=800000,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=32

Run status group 0 (all jobs):
   READ: bw=3943MiB/s (4135MB/s), 3943MiB/s-3943MiB/s (4135MB/s-4135MB/s), io=97.7GiB (105GB), run=25360-25360msec

Disk stats (read/write):
  ublk-0: ios=793776/0, merge=0/0, ticks=791637/0, in_queue=791637, util=99.66%
```

While _fio_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
          14,25    0,00   17,72    0,00    0,00   68,04

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
nullb0        10794,00       674,62         0,00         0,00        674          0          0
nullb1        10793,00       674,56         0,00         0,00        674          0          0
nullb2        10794,00       674,62         0,00         0,00        674          0          0
nullb3        10793,00       674,56         0,00         0,00        674          0          0
nullb4        10793,00       674,56         0,00         0,00        674          0          0
nullb5        10795,00       674,69         0,00         0,00        674          0          0
ublk-0        32380,00      4047,50         0,00         0,00       4047          0          0
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

#### Building RAID5 upon loop devices and use RAID built for deploying ext4 file system upon it

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
filename:       /lib/modules/6.7.5/kernel/drivers/block/loop.ko
license:        GPL
alias:          block-major-7-*
alias:          char-major-10-237
alias:          devname:loop-control
vermagic:       6.7.5 SMP preempt mod_unload
name:           loop
intree:         Y
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

##### Building RAID5

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

We see 365MiB capable a new file system mounted to our mountpoint

##### Check it out with performing IO operations

Let us perform IO operations to check it out.

For the beginning we try to use dd utility performing sequential write operations to a file from the file system we have built upon __RAID5__. We're going to write 200MiB of data, each block being 4KiB long at a time of write IO:

```markdown
# dd if=/dev/random of=raid5ext4mp/a.dat oflag=direct bs=4K count=51200 status=progress
202997760 bytes (203 MB, 194 MiB) copied, 17 s, 11.9 MB/s
51200+0 records in
51200+0 records out
209715200 bytes (210 MB, 200 MiB) copied, 17.5307 s, 12.0 MB/s
```

If we take a look at _iostat_'s measurements while _dd_ is working we will see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           8,86    0,00    6,71   11,39    0,00   73,04

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
loop0          4052,00        36,73        35,00         0,00         36         34          0
loop1          4153,00        36,96        35,20         0,00         36         35          0
loop2          4195,00        36,83        34,96         0,00         36         34          0
ublk-0         2836,00         0,00        11,04         0,00          0         11          0
...
```

Let us do IO write operations by means of fio utility and see how it would go. The most important thing here is that we want to check if data corruption takes place, therefore fio is going to be configured with data verification option. File size will be constrained to 100MiB for this test, write operations will be randomly distributed within these 100MiB space, see:

```markdown
# fio --filename=raid5ext4mp/b.dat --filesize=100M --direct=1 --rw=randrw --bs=16K --ioengine=libaio --iodepth=8 --numjobs=1 --group_reporting --name=ublk-raid5ext4-example-write-verify-test --eta-newline=1 --verify=xxhash
ublk-raid5ext4-example-write-verify-test: (g=0): rw=randrw, bs=(R) 16.0KiB-16.0KiB, (W) 16.0KiB-16.0KiB, (T) 16.0KiB-16.0KiB, ioengine=libaio, iodepth=8
fio-3.34
Starting 1 process
ublk-raid5ext4-example-write-verify-test: Laying out IO file (1 file / 100MiB)
Jobs: 1 (f=1): [V(1)][-.-%][r=44.4MiB/s,w=25.2MiB/s][r=2841,w=1615 IOPS][eta 00m:00s]
ublk-raid5ext4-example-write-verify-test: (groupid=0, jobs=1): err= 0: pid=96967: Sun Mar 10 18:26:43 2024
  read: IOPS=3078, BW=48.1MiB/s (50.4MB/s)(100MiB/2079msec)
    slat (nsec): min=1839, max=56681, avg=4639.17, stdev=3793.38
    clat (usec): min=72, max=5532, avg=829.30, stdev=745.32
     lat (usec): min=75, max=5537, avg=833.94, stdev=746.49
    clat percentiles (usec):
     |  1.00th=[  106],  5.00th=[  147], 10.00th=[  186], 20.00th=[  258],
     | 30.00th=[  318], 40.00th=[  371], 50.00th=[  437], 60.00th=[  676],
     | 70.00th=[ 1123], 80.00th=[ 1516], 90.00th=[ 2008], 95.00th=[ 2343],
     | 99.00th=[ 2966], 99.50th=[ 3261], 99.90th=[ 3752], 99.95th=[ 4015],
     | 99.99th=[ 5538]
   bw (  KiB/s): min=22848, max=26400, per=50.84%, avg=25040.00, stdev=1602.24, samples=4
   iops        : min= 1428, max= 1650, avg=1565.00, stdev=100.14, samples=4
  write: IOPS=1676, BW=26.2MiB/s (27.5MB/s)(51.1MiB/1950msec); 0 zone resets
    slat (usec): min=8, max=103, avg=15.47, stdev= 6.84
    clat (usec): min=823, max=11038, avg=3426.30, stdev=1051.88
     lat (usec): min=846, max=11050, avg=3441.77, stdev=1051.70
    clat percentiles (usec):
     |  1.00th=[ 1401],  5.00th=[ 1876], 10.00th=[ 2212], 20.00th=[ 2573],
     | 30.00th=[ 2868], 40.00th=[ 3097], 50.00th=[ 3359], 60.00th=[ 3621],
     | 70.00th=[ 3851], 80.00th=[ 4228], 90.00th=[ 4752], 95.00th=[ 5145],
     | 99.00th=[ 6587], 99.50th=[ 7504], 99.90th=[ 8979], 99.95th=[ 9896],
     | 99.99th=[11076]
   bw (  KiB/s): min=24064, max=27616, per=97.50%, avg=26160.00, stdev=1607.77, samples=4
   iops        : min= 1504, max= 1726, avg=1635.00, stdev=100.49, samples=4
  lat (usec)   : 100=0.50%, 250=12.17%, 500=23.62%, 750=5.04%, 1000=3.37%
  lat (msec)   : 2=17.26%, 4=29.50%, 10=8.53%, 20=0.01%
  cpu          : usr=4.52%, sys=2.60%, ctx=7353, majf=0, minf=103
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=99.9%, 16=0.0%, 32=0.0%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.1%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     issued rwts: total=6400,3270,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=8

Run status group 0 (all jobs):
   READ: bw=48.1MiB/s (50.4MB/s), 48.1MiB/s-48.1MiB/s (50.4MB/s-50.4MB/s), io=100MiB (105MB), run=2079-2079msec
  WRITE: bw=26.2MiB/s (27.5MB/s), 26.2MiB/s-26.2MiB/s (27.5MB/s-27.5MB/s), io=51.1MiB (53.6MB), run=1950-1950msec

Disk stats (read/write):
  ublk-0: ios=6088/3270, merge=0/0, ticks=5182/11130, in_queue=16312, util=95.39%
```

While _fio_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
          10,68    0,00   20,10    1,38    0,00   67,84

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
loop0          2918,00        24,07        36,54         0,00         24         36          0
loop1          2904,00        24,30        36,54         0,00         24         36          0
loop2          2850,00        22,83        35,05         0,00         22         35          0
ublk-0         1923,00        14,30        66,99         0,00         14         66          0
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
