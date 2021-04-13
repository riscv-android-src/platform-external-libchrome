#!/usr/bin/env python3
# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Script to generate cros/upstream branch.

Usage: update_upstream.py [--delete | --forward | --all]

--delete: Run delete stage to delete unnecessary files. Create one single commit
to delete unnecessary files.
--forward: Run forward stage to update to newer version. Create a series commits
to update cros/upstream to newer revision.
--all: Run all stages. equals to --delete --forward
"""

import argparse
import subprocess
import sys

import filter_config
import filtered_utils
import filters
import lazytree
import utils


def delete(current):
    """ Deletes unnecessary files for new filters.

    Returns a string containing the commit after delete.

    Args:
        current: current cros/upstream commit hash.
    """
    current_metadata = filtered_utils.get_metadata(current)
    original_commit = current_metadata.original_commit_cursor
    new_filter = filters.Filter(filter_config.WANT, filter_config.WANT_EXCLUDE,
                                filter_config.KEEP, filter_config.KEEP_EXCLUDE)
    tree = lazytree.LazyTree(current_metadata.tree)
    current_files = utils.get_file_list(current)

    for f in current_files:
        if not new_filter.want_file(f.path):
            del tree[f.path]

    if tree.hash() == current_metadata.tree:
        return current

    msg = b'Remove unnecessary files due to filter change\n\n%s: %s' % (
        filtered_utils.CROS_LIBCHROME_CURRENT_COMMIT, original_commit)
    new_commit = subprocess.check_output(
        ['git', 'commit-tree', '-p', current,
         tree.hash()], input=msg).strip()
    return new_commit.decode('ascii')


def forward(current, target):
    """ Forwards to given upstream Chromium commit.

    Returns a string containing the commit after forward.

    Args:
        current: current cros/upstream commit hash.
        target: target commit in Chromium src tree.
    """
    proc = subprocess.run([
        'libchrome_tools/developer-tools/uprev/generate_filtered_tree.py',
        current,
        target,
        '--verbose',
    ],
                          stdout=subprocess.PIPE)
    assert proc.returncode == 0
    return proc.stdout.strip().decode('ascii')


def main():
    # Init args
    parser = argparse.ArgumentParser(
        description='Generate Libchrome Upstream Branch')
    parser.add_argument(
        'current',
        metavar='current',
        type=str,
        help='commit hash to start from, usually use cros/upstream.')
    parser.add_argument(
        'target',
        metavar='target',
        type=str,
        help='commit hash in browser tot. only useful if --forward is enabled.')

    parser.add_argument('--all',
                        dest='all',
                        action='store_const',
                        const=True,
                        default=False,
                        help='Run all stages.')
    parser.add_argument('--delete',
                        dest='delete',
                        action='store_const',
                        const=True,
                        default=False,
                        help='Run delete files stage.')
    parser.add_argument('--forward',
                        dest='forward',
                        action='store_const',
                        const=True,
                        default=False,
                        help='Run forward to <target> stage.')

    args = parser.parse_args(sys.argv[1:])

    current = args.current
    target = args.target

    assert args.all or args.delete or args.forward

    if args.all or args.delete:
        current = delete(current)

    if args.all or args.forward:
        current = forward(current, target)

    print(current)


if __name__ == '__main__':
    main()
