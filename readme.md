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
$ sudo out/default/build/Debug/ublksh/ublksh
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
4191158272 bytes (4.2 GB, 3.9 GiB) copied, 14 s, 299 MB/s
4096+0 records in
4096+0 records out
4294967296 bytes (4.3 GB, 4.0 GiB) copied, 14.3623 s, 299 MB/s
```

Let us risk to do sequential read operations by means of fio utility:

```markdown
# fio --filename=/dev/ublk-0 --direct=1 --rw=read --bs=4k --ioengine=libaio --iodepth=32 --numjobs=1 --group_reporting --name=ublk-raid0-example-read-test --eta-newline=1 --readonly
ublk-raid0-example-read-test: (g=0): rw=read, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=32
fio-3.36
Starting 1 process
Jobs: 1 (f=1): [R(1)][9.7%][r=138MiB/s][r=35.4k IOPS][eta 00m:28s]
Jobs: 1 (f=1): [R(1)][16.7%][r=136MiB/s][r=34.8k IOPS][eta 00m:25s]
Jobs: 1 (f=1): [R(1)][23.3%][r=136MiB/s][r=34.8k IOPS][eta 00m:23s]
Jobs: 1 (f=1): [R(1)][30.0%][r=131MiB/s][r=33.4k IOPS][eta 00m:21s]
Jobs: 1 (f=1): [R(1)][36.7%][r=136MiB/s][r=34.7k IOPS][eta 00m:19s]
Jobs: 1 (f=1): [R(1)][43.3%][r=141MiB/s][r=36.1k IOPS][eta 00m:17s]
Jobs: 1 (f=1): [R(1)][50.0%][r=136MiB/s][r=34.8k IOPS][eta 00m:15s]
Jobs: 1 (f=1): [R(1)][56.7%][r=131MiB/s][r=33.5k IOPS][eta 00m:13s]
Jobs: 1 (f=1): [R(1)][63.3%][r=133MiB/s][r=34.1k IOPS][eta 00m:11s]
Jobs: 1 (f=1): [R(1)][70.0%][r=138MiB/s][r=35.3k IOPS][eta 00m:09s]
Jobs: 1 (f=1): [R(1)][76.7%][r=143MiB/s][r=36.6k IOPS][eta 00m:07s]
Jobs: 1 (f=1): [R(1)][83.3%][r=128MiB/s][r=32.6k IOPS][eta 00m:05s]
Jobs: 1 (f=1): [R(1)][90.3%][r=118MiB/s][r=30.3k IOPS][eta 00m:03s]
Jobs: 1 (f=1): [R(1)][96.8%][r=119MiB/s][r=30.5k IOPS][eta 00m:01s]
Jobs: 1 (f=1): [R(1)][100.0%][r=118MiB/s][r=30.2k IOPS][eta 00m:00s]
ublk-raid0-example-read-test: (groupid=0, jobs=1): err= 0: pid=78393: Thu Jun 20 15:53:32 2024
  read: IOPS=34.0k, BW=133MiB/s (139MB/s)(4096MiB/30822msec)
    slat (nsec): min=838, max=1395.3k, avg=2957.56, stdev=4236.64
    clat (usec): min=27, max=13863, avg=936.91, stdev=623.96
     lat (usec): min=40, max=13865, avg=939.87, stdev=624.15
    clat percentiles (usec):
     |  1.00th=[   92],  5.00th=[  174], 10.00th=[  265], 20.00th=[  412],
     | 30.00th=[  545], 40.00th=[  676], 50.00th=[  824], 60.00th=[  988],
     | 70.00th=[ 1156], 80.00th=[ 1385], 90.00th=[ 1745], 95.00th=[ 2089],
     | 99.00th=[ 2802], 99.50th=[ 3130], 99.90th=[ 4080], 99.95th=[ 4948],
     | 99.99th=[11207]
   bw (  KiB/s): min=118216, max=149416, per=100.00%, avg=136347.02, stdev=8702.28, samples=61
   iops        : min=29554, max=37354, avg=34086.75, stdev=2175.57, samples=61
  lat (usec)   : 50=0.01%, 100=1.40%, 250=7.81%, 500=17.49%, 750=18.39%
  lat (usec)   : 1000=15.82%
  lat (msec)   : 2=33.16%, 4=5.82%, 10=0.10%, 20=0.01%
  cpu          : usr=7.74%, sys=17.41%, ctx=997303, majf=0, minf=42
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=100.0%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.1%, 64=0.0%, >=64=0.0%
     issued rwts: total=1048576,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=32

Run status group 0 (all jobs):
   READ: bw=133MiB/s (139MB/s), 133MiB/s-133MiB/s (139MB/s-139MB/s), io=4096MiB (4295MB), run=30822-30822msec

Disk stats (read/write):
  ublk-0: ios=1043375/0, sectors=8347000/0, merge=0/0, ticks=970237/0, in_queue=970237, util=99.69%
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
filename:       /lib/modules/6.9.4-linux/kernel/drivers/block/null_blk/null_blk.ko
author:         Jens Axboe <axboe@kernel.dk>
license:        GPL
vermagic:       6.9.4-linux SMP preempt mod_unload
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
fio-3.36
Starting 1 process
Jobs: 1 (f=1): [R(1)][15.0%][r=4923MiB/s][r=39.4k IOPS][eta 00m:17s]
Jobs: 1 (f=1): [R(1)][25.0%][r=4783MiB/s][r=38.3k IOPS][eta 00m:15s]
Jobs: 1 (f=1): [R(1)][35.0%][r=4971MiB/s][r=39.8k IOPS][eta 00m:13s]
Jobs: 1 (f=1): [R(1)][45.0%][r=4842MiB/s][r=38.7k IOPS][eta 00m:11s]
Jobs: 1 (f=1): [R(1)][55.0%][r=4777MiB/s][r=38.2k IOPS][eta 00m:09s]
Jobs: 1 (f=1): [R(1)][65.0%][r=4889MiB/s][r=39.1k IOPS][eta 00m:07s]
Jobs: 1 (f=1): [R(1)][75.0%][r=4894MiB/s][r=39.1k IOPS][eta 00m:05s]
Jobs: 1 (f=1): [R(1)][85.0%][r=5276MiB/s][r=42.2k IOPS][eta 00m:03s]
Jobs: 1 (f=1): [R(1)][95.0%][r=4950MiB/s][r=39.6k IOPS][eta 00m:01s]
Jobs: 1 (f=1): [R(1)][100.0%][r=4946MiB/s][r=39.6k IOPS][eta 00m:00s]
ublk-raid1-example-read-test: (groupid=0, jobs=1): err= 0: pid=76443: Thu Jun 20 15:47:56 2024
  read: IOPS=38.9k, BW=4864MiB/s (5100MB/s)(97.7GiB/20560msec)
    slat (usec): min=2, max=2754, avg= 8.05, stdev= 7.01
    clat (usec): min=143, max=7577, avg=813.44, stdev=290.24
     lat (usec): min=156, max=7585, avg=821.50, stdev=291.96
    clat percentiles (usec):
     |  1.00th=[  461],  5.00th=[  510], 10.00th=[  537], 20.00th=[  586],
     | 30.00th=[  627], 40.00th=[  676], 50.00th=[  734], 60.00th=[  816],
     | 70.00th=[  906], 80.00th=[ 1020], 90.00th=[ 1188], 95.00th=[ 1336],
     | 99.00th=[ 1696], 99.50th=[ 1926], 99.90th=[ 2737], 99.95th=[ 3163],
     | 99.99th=[ 5866]
   bw (  MiB/s): min= 4103, max= 5285, per=100.00%, avg=4866.40, stdev=255.14, samples=41
   iops        : min=32830, max=42282, avg=38931.22, stdev=2041.06, samples=41
  lat (usec)   : 250=0.01%, 500=3.86%, 750=48.42%, 1000=26.19%
  lat (msec)   : 2=21.12%, 4=0.39%, 10=0.02%
  cpu          : usr=7.42%, sys=36.14%, ctx=379791, majf=0, minf=1034
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=99.9%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.1%, 64=0.0%, >=64=0.0%
     issued rwts: total=800000,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=32

Run status group 0 (all jobs):
   READ: bw=4864MiB/s (5100MB/s), 4864MiB/s-4864MiB/s (5100MB/s-5100MB/s), io=97.7GiB (105GB), run=20560-20560msec

Disk stats (read/write):
  ublk-0: ios=792849/0, sectors=202969344/0, merge=0/0, ticks=634796/0, in_queue=634796, util=99.30%
```

While _fio_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
          15,72    0,00   28,27    0,00    0,00   56,01

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
nullb0        12572,00       785,75         0,00         0,00        785          0          0
nullb1        12573,00       785,81         0,00         0,00        785          0          0
nullb2        12573,00       785,81         0,00         0,00        785          0          0
nullb3        12573,00       785,81         0,00         0,00        785          0          0
nullb4        12573,00       785,81         0,00         0,00        785          0          0
nullb5        12572,00       785,75         0,00         0,00        785          0          0
ublk-0        37716,00      4714,50         0,00         0,00       4714          0          0
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
filename:       /lib/modules/6.9.4-linux/kernel/drivers/block/loop.ko
license:        GPL
alias:          block-major-7-*
alias:          char-major-10-237
alias:          devname:loop-control
vermagic:       6.9.4-linux SMP preempt mod_unload
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
192331776 байтов (192 MB, 183 MiB) скопировано, 8 s, 24,0 MB/s
51200+0 записей получено
51200+0 записей отправлено
209715200 байтов (210 MB, 200 MiB) скопировано, 8,69378 s, 24,1 MB/s
```

If we take a look at _iostat_'s measurements while _dd_ is working we will see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
          13,99    0,00   18,65    7,25    0,00   60,10

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
loop0          9208,00        78,12        78,12         0,00         78         78          0
loop1          9185,00        77,81        77,80         0,00         77         77          0
loop2          9214,00        78,22        78,22         0,00         78         78          0
ublk-0         6310,00         0,00        24,65         0,00          0         24          0
...
```

Let us do IO write operations by means of fio utility and see how it would go. The most important thing here is that we want to check if data corruption takes place, therefore fio is going to be configured with data verification option. File size will be constrained to 100MiB for this test, write operations will be randomly distributed within these 100MiB space, see:

```markdown
# fio --filename=raid5ext4mp/b.dat --filesize=100M --direct=1 --rw=randrw --bs=16K --ioengine=libaio --iodepth=8 --numjobs=1 --group_reporting --name=ublk-raid5ext4-example-write-verify-test --eta-newline=1 --verify=xxhash
ublk-raid5ext4-example-write-verify-test: (g=0): rw=randrw, bs=(R) 16.0KiB-16.0KiB, (W) 16.0KiB-16.0KiB, (T) 16.0KiB-16.0KiB, ioengine=libaio, iodepth=8
fio-3.36
Starting 1 process
ublk-raid5ext4-example-write-verify-test: Laying out IO file (1 file / 100MiB)
ublk-raid5ext4-example-write-verify-test: (groupid=0, jobs=1): err= 0: pid=84063: Thu Jun 20 16:06:59 2024
  read: IOPS=13.4k, BW=209MiB/s (219MB/s)(100MiB/478msec)
    slat (nsec): min=1624, max=421595, avg=6061.59, stdev=7730.37
    clat (usec): min=36, max=1814, avg=245.64, stdev=149.04
     lat (usec): min=39, max=1818, avg=251.70, stdev=150.60
    clat percentiles (usec):
     |  1.00th=[   60],  5.00th=[   83], 10.00th=[  102], 20.00th=[  135],
     | 30.00th=[  159], 40.00th=[  184], 50.00th=[  208], 60.00th=[  241],
     | 70.00th=[  281], 80.00th=[  338], 90.00th=[  424], 95.00th=[  523],
     | 99.00th=[  758], 99.50th=[  824], 99.90th=[ 1385], 99.95th=[ 1631],
     | 99.99th=[ 1811]
  write: IOPS=8406, BW=131MiB/s (138MB/s)(51.1MiB/389msec); 0 zone resets
    slat (usec): min=8, max=232, avg=18.79, stdev=10.93
    clat (usec): min=147, max=3556, avg=645.45, stdev=274.97
     lat (usec): min=157, max=3577, avg=664.24, stdev=277.65
    clat percentiles (usec):
     |  1.00th=[  239],  5.00th=[  322], 10.00th=[  371], 20.00th=[  433],
     | 30.00th=[  486], 40.00th=[  537], 50.00th=[  586], 60.00th=[  660],
     | 70.00th=[  725], 80.00th=[  824], 90.00th=[  979], 95.00th=[ 1123],
     | 99.00th=[ 1549], 99.50th=[ 1893], 99.90th=[ 2311], 99.95th=[ 2802],
     | 99.99th=[ 3556]
  lat (usec)   : 50=0.19%, 100=6.13%, 250=35.51%, 500=31.64%, 750=16.63%
  lat (usec)   : 1000=6.54%
  lat (msec)   : 2=3.26%, 4=0.10%
  cpu          : usr=16.56%, sys=17.61%, ctx=9029, majf=0, minf=102
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=99.9%, 16=0.0%, 32=0.0%, >=64=0.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.1%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     issued rwts: total=6400,3270,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=8

Run status group 0 (all jobs):
   READ: bw=209MiB/s (219MB/s), 209MiB/s-209MiB/s (219MB/s-219MB/s), io=100MiB (105MB), run=478-478msec
  WRITE: bw=131MiB/s (138MB/s), 131MiB/s-131MiB/s (138MB/s-138MB/s), io=51.1MiB (53.6MB), run=389-389msec

Disk stats (read/write):
  ublk-0: ios=2333/2450, sectors=74656/78400, merge=0/0, ticks=618/1456, in_queue=2074, util=72.73%
```

While _fio_ is working run _iostat_ utility to see IO progress at block devices:

```bash
$ iostat -ym 1
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
          16,32    0,00   22,11    1,67    0,00   59,90

Device             tps    MB_read/s    MB_wrtn/s    MB_dscd/s    MB_read    MB_wrtn    MB_dscd
loop0          8132,00        68,31        98,51         0,00         68         98          0
loop1          8068,00        68,78        99,14         0,00         68         99          0
loop2          7974,00        69,45        98,86         0,00         69         98          0
ublk-0         4927,00        36,80       138,60         0,00         36        138          0
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
