#!/usr/bin/python2

"""
Copyright (C) 2017, PARSA EPFL

Authors:
    Klim S. Kireev <klim.s.kireev@gmail.com>

This work is licensed under the terms of the GNU GPL, version 2 or later. See
the COPYING file in the top-level directory.
"""

#TODO: To limit default tree depth

import os
import argparse
import datetime

import resource, sys
resource.setrlimit(resource.RLIMIT_STACK, (2**29,-1))
sys.setrecursionlimit(10**6) # because default limit is too low and we need to increase it

scan_cmd = "find */*-sn -type f -print | xargs -L 1 qemu-img info | grep -a -e 'image:' -e 'backing file:' | sed -e 's@image:\ @@g' -e  's@backing file:\ @@g'"
memory = 'mem'

class Snapshot(object):

    def __init__(self, image, backing=None):

        self.image = os.path.abspath(image)
        self.need_update = False

        if backing == None:
            self.backing = None
            self.name = ''
            self.back_name = None

            self.disk = os.path.split(self.image)[1]
            self.back_base = None
            return

        self.backing = os.path.normpath(backing)
        self.name = os.path.split(os.path.split(self.image)[0])[1]
        
        if os.path.split(self.backing)[1] == os.path.split(root_image)[1]:
            self.back_name = ''
            self.back_base = os.path.split(self.backing)[0] # if back image is root image
        else:
            self.back_name = os.path.split(os.path.split(self.backing)[0])[1]
            self.back_base = os.path.split(os.path.split(self.backing)[0])[0]

        self.disk = os.path.split(self.image)[1]
        if base_dir != self.back_base :
            self.need_update = True

    def __str__(self):
        return str(self.name)

    def __repr__(self):
        return str(self)

    def update(self):
        if self.name == '':
            return
        if self.back_name == '':
            newback = os.path.join(base_dir, os.path.split(root_image)[1])
        else:
            newback = os.path.join(os.path.join(base_dir, self.back_name), self.disk)
        rebase_cmd = " ".join(['qemu-img rebase -u -b', newback, self.image])
        os.system(rebase_cmd)

    def delete(self):
        if self.name == '':
            return
        filename = os.path.join(base_dir, self.name);
        rm_cmd = " ".join(['rm -rf', filename])
        print rm_cmd # Its untested properly, if you want delete just copy and execute it

    def compress(self, out):
        if self.name == '':
            return
        comp_cmd = " ".join(['tar -rvvf', out, self.name])
        os.system(comp_cmd)

    def info(self):
        stmem = os.stat(os.path.join(base_dir, self.name + '/mem'))
        stdisk = os.stat(self.image)
        print
        print 'Snapshot ' + self.name
        print 'Disk size: %d' % stdisk.st_size
        print 'Memory size: %d' % stmem.st_size
        print 'Date: ', datetime.datetime.fromtimestamp(stmem.st_mtime)
        

class SnapTree(object):

    class SnapNode(object):

        def __init__(self, snap, parent=None):
            self.snap = snap
            self.parent = parent
            self.children = list()

        def __str__(self):
            return self.snap.name

        def __repr__(self):
            return str(self)

        def add_children(self, snap_list):
            child_list = filter((lambda x: x.back_name == self.snap.name), snap_list)
            for i in child_list:
                self.children.append(SnapTree.SnapNode(i, self))
            for c in self.children:
                c.add_children(snap_list)

        def find(self, name):
            if self.snap.name == name:
                return self
            else:
                for i in self.children:
                    res = i.find(name)
                    if res != None:
                        return res
            return None
        
        def get_chain(self):
            chain = [self.snap]
            if self.parent == None:
                return chain
            return chain + self.parent.get_chain()
        
        def dump(self, indent):
            if indent > 0:
                print ' ' * 8 * (indent - 1) + '^-------' + str(self)
            for i in self.children:
                i.dump(indent + 1)
                
        def delete(self):
            map((lambda x: x.delete()), self.children)
            self.snap.delete()

        def rec_update(self):
            self.snap.update()
            map((lambda x: x.rec_update()), self.children)

        def rec_compress(self, out):
            self.snap.compress(out)
            map((lambda x: x.rec_compress(out)), self.children)

    def __init__(self, snap_list):
        self.root = self.SnapNode(Snapshot(root_image))
        self.root.add_children(snap_list)

    def dump(self, name=None):
        if name == None:
            print 'root'
            self.root.dump(0)
            return
        snap = self.root.find(name)
        snap.snap.info()

    def get_chain(self, name):
        snap = self.root.find(name)
        if snap == None:
            return []
        return snap.get_chain()

    def update(self, name=None):
        if name == None:
            self.root.rec_update()
            return
        end = self.root.find(name)
        if end == None:
            return
        chain = end.get_chain()
        map((lambda x: x.update()), chain)
    
    def compress(self, out, name=None):
        if name == None:
            self.root.rec_compress(out)
            return
        end = self.root.find(name)
        if end == None:
            return
        chain = end.get_chain()
        map((lambda x: x.compress(out)), chain)
    
    def delete(self, name=None):
        if name == None:
            self.root.delete()
            return
        snap = self.root.find(name)
        snap.delete()

def action_update(args):

    if args.snapshots == []:
        snap_tree.update()
    map((lambda x: snap_tree.update(x)), args.snapshots)

    return

def action_delete(args):

    if args.snapshots == []:
        snap_tree.delete()
    map((lambda x: snap_tree.delete(x)), args.snapshots)

    return

def action_info(args):

    if args.snapshots == []:
        snap_tree.dump()
    map((lambda x: snap_tree.dump(x)), args.snapshots)
    return 

def action_compress(args):

    out = args.output
    if args.output == None:
        out = 'snapshots.tar'
    
    if args.snapshots == []:
        snap_tree.compress(out)
    map((lambda x: snap_tree.compress(out, x)), args.snapshots)

    os.system("gzip " + out)
    return

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", help="Increase output verbosity",
                        action="store_true")
    parser.add_argument("--dry-run", help="Do nothing just print commands",
                        action="store_true")
    subparsers = parser.add_subparsers(help="sub-command help")
    
    parser_update = subparsers.add_parser('update', help="Change absolute path in all snapshots in chain")
    parser_update.add_argument("root_image", help="The root image of snapshot tree.")
    parser_update.add_argument("snapshots", nargs='*', help="The name of the required snapshots. By default, update whole tree")
    parser_update.set_defaults(func=action_update)

    parser_delete = subparsers.add_parser('delete', help="Delete the snapshot and all snapshots based on it.")
    parser_delete.add_argument("root_image", help="The root image of snapshot tree.")
    parser_delete.add_argument("snapshots", nargs='*', help="The name of the required snapshots. By default delete whole tree")
    parser_delete.set_defaults(func=action_delete)

    parser_info = subparsers.add_parser('info', help="Display information about snapshot")
    parser_info.add_argument("root_image", help="The root image of snapshot tree.")
    parser_info.add_argument("snapshots", nargs='*', help="The name of the required snapshots. By default, print whole tree")
    parser_info.set_defaults(func=action_info)

    parser_info = subparsers.add_parser('compress', help="Compress snapshots to single file")
    parser_info.add_argument("root_image", help="The root image of snapshot tree.")
    parser_info.add_argument("snapshots", nargs='*', help="The name of the required snapshots. By default, compress whole tree")
    parser_info.add_argument('-o', '--output', help="Output file. By default, snapshots.tar.gz")
    parser_info.set_defaults(func=action_compress)

    args = parser.parse_args()

    global root_image
    root_image = os.path.abspath(args.root_image)

    global base_dir
    base_dir = os.path.split(root_image)[0]

    os.chdir(base_dir)
    raw_list = os.popen(scan_cmd).read().splitlines()
    snap_list = [Snapshot(raw_list[i], raw_list[i+1]) for i in range(0, len(raw_list) - 1, 2)]

    global snap_tree
    snap_tree = SnapTree(snap_list)
    
    args.func(args)

if __name__ == "__main__":
    main()
