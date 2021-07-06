#!/usr/bin/env python3
# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import subprocess
import sys
import tempfile

TEMPLATE = '''
# THIS FILE IS GENERATED. DO NOT EDIT MANUALLY.


ALL_BOARDS = %s
DEFAULT_BOARDS = %s

BOARDS_MAPPING = {
    'all': ALL_BOARDS,
    'default': DEFAULT_BOARDS,
}

# Not used. For reference only.
DEFAULT_BOARDS_REASON = %s
'''

_BUILDER_CONFIG_PATH = '/mnt/host/source/infra/config/generated/builder_configs.cfg'
_VERSION_REMOVE_RE = re.compile('(.*?)(-[0-9.]+)?(-r[0-9]+)?$')


def main():
    srcdir = os.path.dirname(__file__)

    all_cqs = subprocess.check_output([
        'jq',
        '.[][] | select (.id.name == "cq-orchestrator") | .orchestrator.childSpecs[].name',
        '-r',
        _BUILDER_CONFIG_PATH,
    ])
    all_cqs = all_cqs.split(b'\n')
    all_criticals = subprocess.check_output([
        'jq', '.[][] | select (.general.critical == true) | .id.name', '-r',
        _BUILDER_CONFIG_PATH
    ])
    all_criticals = all_criticals.split(b'\n')
    critical_cqs = set(all_cqs).intersection(set(all_criticals))
    all_boards = []
    for cq in critical_cqs:
        cq = cq.strip()
        if not cq:
            continue
        cq = cq.decode('ascii')
        assert cq.endswith('-cq')
        board = cq[:-3]
        # In builder_configs, generic boards has asan builders, which we don't
        # need.
        if re.match('(amd64|arm|arm64)-generic.+', board):
            continue
        if board == 'chromite':
            continue
        all_boards.append(board)
    all_boards = sorted(all_boards)
    print('%d boards found %s' % (len(all_boards), all_boards))

    unbuilt_boards = []
    for idx, board in enumerate(all_boards, start=1):
        print('Pre-Checking %s (%d/%d)' % (board, idx, len(all_boards)),
              flush=True)
        try:
            subprocess.check_call(['equery-' + board, 'd', 'libchrome'],
                                  stdout=subprocess.DEVNULL)
        except:
            unbuilt_boards.append(board)

    # Use parallel checker to run build_packags.
    if unbuilt_boards:
        args = []
        for board in unbuilt_boards:
            args += ['-b', board]
        with tempfile.TemporaryDirectory() as d:
            subprocess.check_call([
                os.path.join(srcdir, 'parallel_packages_checker.py'), '-d', d,
                '--allow-output-directory-exists'
            ] + args)

    default_boards = []
    libchrome_users = set()
    libchrome_users_by_board = {}
    for idx, board in enumerate(all_boards, start=1):
        print('Checking %s (%d/%d)' % (board, idx, len(all_boards)),
              end=' ',
              flush=True)
        out = subprocess.check_output(['equery-' + board, 'd', 'libchrome'])
        out_lines = out.decode('utf-8').split('\n')
        packages_with_libchrome_deps = set()
        for line in out_lines:
            if not line:
                continue
            # Equery has lines begin with ' * ' for progress-purpose
            # information, which is not the result. All results have no leading
            # space.
            if line[0] == ' ':
                continue
            pkgname = _VERSION_REMOVE_RE.match(line.split(' ')[0]).group(1)
            packages_with_libchrome_deps.add(pkgname)
            libchrome_users.add(pkgname)
        print('%d packages / all boards total %d pacakges' %
              (len(packages_with_libchrome_deps), len(libchrome_users)))
        libchrome_users_by_board[board] = packages_with_libchrome_deps
    print('Total of %d packages depending on libchrome' %
          (len(libchrome_users)))

    # Use greedy algorithm to find a sub-optimal minimum boards coverage.
    boards_reason = {}
    while libchrome_users:
        max_board, max_board_cnt = None, 0
        for board in libchrome_users_by_board:
            libchrome_users_by_board[board] = libchrome_users_by_board[
                board].intersection(libchrome_users)
            if len(libchrome_users_by_board[board]) > max_board_cnt:
                max_board, max_board_cnt = board, len(
                    libchrome_users_by_board[board])
        assert max_board
        default_boards.append(max_board)
        libchrome_users.difference_update(libchrome_users_by_board[max_board])
        boards_reason[max_board] = libchrome_users_by_board[max_board]
        del libchrome_users_by_board[max_board]
    print('Recommended coverage: %s' % (default_boards))

    with open(os.path.join(srcdir, 'config.py'), 'w') as f:
        f.write(TEMPLATE %
                (repr(all_boards), repr(default_boards), repr(boards_reason)))

    subprocess.check_call([
        '/mnt/host/depot_tools/yapf', '-i',
        os.path.join(srcdir, 'config.py'), '--style=google'
    ])


if __name__ == '__main__':
    main()
