#!/usr/bin/env python3

import ublk
from cmd import Cmd


class ublksh(Cmd):
    prompt = 'ublksh > '
    master = ublk.Master()

    @staticmethod
    def __parse_args__(arg, sep=' '):
        res = []
        for sub in arg.split(sep):
            if '=' in sub:
                res.append(map(str.strip, sub.split('=', 1)))
        return dict(res)

    @staticmethod
    def __parse_csv_list__(arg):
        return arg.split(',')

    @staticmethod
    def __parse_target_null__(args):
        return ublk.target_null()

    @staticmethod
    def __parse_target_inmem__(args):
        return ublk.target_inmem()

    @staticmethod
    def __parse_target_default__(args):
        target = ublk.target_default()

        try:
            target.path = args['path']
        except KeyError:
            print("No 'path' given for the default target in the arguments")
            raise
        return target

    @staticmethod
    def __parse_target_raid0__(args):
        target = ublk.target_raid0()

        try:
            target.strip_len_sectors = int(args['strip_len_sectors'])
        except KeyError:
            print("No 'strip_len_sectors' given in the arguments")
            raise
        except ValueError:
            print("'strip_len_sectors' given cannot be converted to sectors")
            raise

        try:
            target.paths = ublksh.__parse_csv_list__(args['paths'])
        except KeyError:
            print("No 'paths' given for the raid0 target in the arguments")
            raise

        return target

    @staticmethod
    def __parse_target_raid1__(args):
        target = ublk.target_raid1()

        try:
            target.read_len_sectors_per_path = int(
                args.get('read_len_sectors_per_path', 0))
        except ValueError:
            print(
                "'read_len_sectors_per_path' given for the raid1 target cannot"
                " be converted to sectors")
            raise

        try:
            target.paths = ublksh.__parse_csv_list__(args['paths'])
        except KeyError:
            print("No 'paths' given for the raid1 target in the arguments")
            raise

        return target

    @staticmethod
    def __parse_target_raid4__(args):
        target = ublk.target_raid4()

        try:
            target.strip_len_sectors = int(args['strip_len_sectors'])
        except KeyError:
            print("No 'strip_len_sectors' given for the raid4 target in "
                  "the arguments")
            raise
        except ValueError:
            print(
                "'strip_len_sectors' given for the raid4 target cannot be"
                " converted to sectors")
            raise

        try:
            target.data_paths = ublksh.__parse_csv_list__(args['data_paths'])
        except KeyError:
            print("No 'data_paths' given for the raid4 target "
                  "in the arguments")
            raise

        try:
            target.parity_path = args['parity_path']
        except KeyError:
            print("No 'parity_path' given for the raid4 target "
                  "in the arguments")
            raise

        return target

    @staticmethod
    def __parse_target_raid5__(args):
        target = ublk.target_raid5()

        try:
            target.strip_len_sectors = int(args['strip_len_sectors'])
        except KeyError:
            print("No 'strip_len_sectors' given for the raid5 target in "
                  "the arguments")
            raise
        except ValueError:
            print(
                "'strip_len_sectors' given for the raid5 target cannot be"
                " converted to sectors")
            raise

        try:
            target.paths = ublksh.__parse_csv_list__(args['paths'])
        except KeyError:
            print("No 'paths' given for the raid5 target "
                  "in the arguments")
            raise

        return target

    @staticmethod
    def __parse_targets_raid1s__(arg):
        targets = []

        for raid1 in arg.split(':'):
            raid1 = raid1.replace('\"', '')
            targets.append(ublksh.__parse_target_raid1__(
                ublksh.__parse_args__(raid1, ';')))

        return targets

    @staticmethod
    def __parse_targets_raid4s__(arg):
        targets = []

        for raid4 in arg.split(':'):
            raid4 = raid4.replace('\"', '')
            targets.append(ublksh.__parse_target_raid4__(
                ublksh.__parse_args__(raid4, ';')))

        return targets

    @staticmethod
    def __parse_targets_raid5s__(arg):
        targets = []

        for raid5 in arg.split(':'):
            raid5 = raid5.replace('\"', '')
            targets.append(ublksh.__parse_target_raid5__(
                ublksh.__parse_args__(raid5, ';')))

        return targets

    @staticmethod
    def __parse_target_raid10__(args):
        target = ublk.target_raid10()

        try:
            target.strip_len_sectors = int(args['strip_len_sectors'])
        except KeyError:
            print("No 'strip_len_sectors' given in the arguments")
            raise
        except ValueError:
            print("'strip_len_sectors' given cannot be converted to sectors")
            raise

        try:
            target.raid1s = ublksh.__parse_targets_raid1s__(args['raid1s'])
        except KeyError:
            print("No 'raid1s' given for the raid10 target in the arguments")
            raise

        return target

    @staticmethod
    def __parse_target_raid40__(args):
        target = ublk.target_raid40()

        try:
            target.strip_len_sectors = int(args['strip_len_sectors'])
        except KeyError:
            print("No 'strip_len_sectors' given in the arguments")
            raise
        except ValueError:
            print("'strip_len_sectors' given cannot be converted to sectors")
            raise

        try:
            target.raid4s = ublksh.__parse_targets_raid4s__(args['raid4s'])
        except KeyError:
            print("No 'raid4s' given for the raid40 target in the arguments")
            raise

        return target

    @staticmethod
    def __parse_target_raid50__(args):
        target = ublk.target_raid50()

        try:
            target.strip_len_sectors = int(args['strip_len_sectors'])
        except KeyError:
            print("No 'strip_len_sectors' given in the arguments")
            raise
        except ValueError:
            print("'strip_len_sectors' given cannot be converted to sectors")
            raise

        try:
            target.raid5s = ublksh.__parse_targets_raid5s__(args['raid5s'])
        except KeyError:
            print("No 'raid5s' given for the raid50 target in the arguments")
            raise

        return target

    def do_exit(self, arg):
        print("Bye")
        return True

    def help_exit(self):
        print('Quits the application. Shorthand: x q Ctrl-D.')

    def do_target_create(self, arg):
        args = ublksh.__parse_args__(arg)

        param = ublk.target_create_param()

        try:
            param.name = args['name']
        except KeyError:
            print("No 'name' given in the arguments")
            return

        try:
            param.capacity_sectors = int(args['capacity_sectors'])
        except KeyError:
            print("No 'capacity_sectors' given in the arguments")
            return
        except ValueError:
            print("'capacity_sectors' given cannot be converted to sectors")
            return

        cache_len_sectors = 0
        try:
            cache_len_sectors = int(args.get('cache_len_sectors', 0))
        except ValueError:
            print(
                "'cache_len_sectors' given cannot be"
                " converted to sectors")
            raise

        cache_write_through_enable = True
        try:
            cache_write_through_enable = int(
                args.get('cache_write_through_enable', True))
        except ValueError:
            print(
                "'cache_write_through_enable' given cannot be converted to"
                " True or False")
            raise

        if cache_len_sectors > 0:
            cache = ublk.cache_cfg()

            cache.len_sectors = cache_len_sectors
            cache.write_through_enable = cache_write_through_enable

            param.cache = cache

        target_type = str()

        try:
            target_type = args['type']
        except KeyError:
            print("No 'type' given for the target in the arguments")
            return

        try:
            if target_type == 'null':
                param.target = ublksh.__parse_target_null__(args)
            elif target_type == 'inmem':
                param.target = ublksh.__parse_target_inmem__(args)
            elif target_type == 'default':
                param.target = ublksh.__parse_target_default__(args)
            elif target_type == 'raid0':
                param.target = ublksh.__parse_target_raid0__(args)
            elif target_type == 'raid1':
                param.target = ublksh.__parse_target_raid1__(args)
            elif target_type == 'raid4':
                param.target = ublksh.__parse_target_raid4__(args)
            elif target_type == 'raid5':
                param.target = ublksh.__parse_target_raid5__(args)
            elif target_type == 'raid10':
                param.target = ublksh.__parse_target_raid10__(args)
            elif target_type == 'raid40':
                param.target = ublksh.__parse_target_raid40__(args)
            elif target_type == 'raid50':
                param.target = ublksh.__parse_target_raid50__(args)
            else:
                print("Unknown type '{}' given for the target in the arguments".format(
                    target_type))
                return
        except BaseException:
            print("Error occurred while parsing command parameters")
            return

        try:
            self.master.create(param)
        except BaseException:
            print("Error occurred while creating the target")
            pass

    def help_target_create(self):
        print("Creates a new target.")

    def do_target_destroy(self, arg):
        args = ublksh.__parse_args__(arg)

        param = ublk.target_destroy_param()

        try:
            param.name = args['name']
        except KeyError:
            print("No 'name' given in the arguments")
            return

        try:
            self.master.destroy(param)
        except BaseException:
            print("Error occurred while destroying the target")
            pass

    def help_target_destroy(self):
        print("Destroys an exising target.")

    def do_bdev_map(self, arg):
        args = ublksh.__parse_args__(arg)

        param = ublk.bdev_map_param()

        try:
            param.bdev_suffix = args['bdev_suffix']
        except KeyError:
            print("No 'bdev_suffix' given in the arguments")
            return

        try:
            param.target_name = args['target_name']
        except KeyError:
            print("No 'target_name' given in the arguments")
            return

        param.read_only = args.get('read_only', False)

        try:
            self.master.map(param)
        except BaseException:
            print("Error occurred while mapping the block device to the target")
            pass

    def help_bdev_map(self):
        print("Maps a new block device to the existing target.")

    def do_bdev_unmap(self, arg):
        args = ublksh.__parse_args__(arg)

        param = ublk.bdev_unmap_param()

        try:
            param.bdev_suffix = args['bdev_suffix']
        except KeyError:
            print("No 'bdev_suffix' given in the arguments")
            return

        try:
            self.master.unmap(param)
        except BaseException:
            print("Error occurred while unmapping the block device from the target")
            pass

    def help_bdev_unmap(self):
        print("Unmaps an exising block device from the target")

    def default(self, inp):
        if inp == 'x' or inp == 'q':
            return self.do_exit(inp)

        print("Default: {}".format(inp))

    do_EOF = do_exit
    help_EOF = help_exit


if __name__ == '__main__':
    ublksh().cmdloop("Type ? to list commands")
