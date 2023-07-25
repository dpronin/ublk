#!/usr/bin/env python3

import ublk
from cmd import Cmd


class ublksh(Cmd):
    prompt = 'ublksh > '
    master = ublk.Master()

    @staticmethod
    def __parse_args__(arg):
        res = []
        for sub in arg.split(' '):
            if '=' in sub:
                res.append(map(str.strip, sub.split('=', 1)))
        return dict(res)

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

        try:
            param.path = args['path']
        except KeyError:
            print("No 'path' given in the arguments")
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
