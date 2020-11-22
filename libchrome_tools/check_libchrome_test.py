#!/usr/bin/env python
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import random
import subprocess
import shutil
import tempfile
import unittest

import check_libchrome_utils

BAD_FILE = 'testdata/check-libchrome-test.cc'
PREFIX = 'check-libchrome-test-' + str(random.randint(0, 10000)) + '/'

TEST_BAD_KEYWORDS = {
    b'DO_NOT_USE_IN_LIBCHROME': 'TEST_PURPOSE',
}


class TestCheckLibchrome(unittest.TestCase):

    def setUp(self):
        self.repo_dir = tempfile.TemporaryDirectory()
        self.orig_dir = os.getcwd()
        os.chdir(self.repo_dir.name)
        subprocess.check_call(['git', 'init'])
        subprocess.check_call(['git', 'config', 'user.name', 'Test'])
        subprocess.check_call(
            ['git', 'config', 'user.email', 'test@chromium.org'])
        subprocess.check_call(
            ['git', 'commit', '-m', 'initial commit', '--allow-empty'])
        os.makedirs(os.path.join(self.repo_dir.name, os.path.dirname(BAD_FILE)))
        shutil.copyfile(os.path.join(self.orig_dir, BAD_FILE),
                        os.path.join(self.repo_dir.name, BAD_FILE))
        subprocess.check_call(['git', 'add', BAD_FILE])
        subprocess.check_call(['git', 'commit', '-m', 'bad commit'])
        self.commit = subprocess.check_output(
            ['git', 'rev-parse', '--verify', 'HEAD']).strip()
        os.chdir(self.orig_dir)

    def tearDown(self):
        os.chdir(self.orig_dir)
        name = self.repo_dir.name
        self.repo_dir.cleanup()

    def build_environ(self, files):
        return {
            'PRESUBMIT_FILES': '\n'.join(files),
            'PRESUBMIT_COMMIT': self.commit,
        }

    def test_check(self):
        os.chdir(self.repo_dir.name)
        self.assertEqual(check_libchrome_utils.check(self.build_environ([])),
                          [])
        self.assertEqual(
            check_libchrome_utils.check(self.build_environ([BAD_FILE]),
                                        TEST_BAD_KEYWORDS),
            [
                'In File testdata/check-libchrome-test.cc, '
                'found b\'DO_NOT_USE_IN_LIBCHROME\' (pattern: '
                'DO_NOT_USE_IN_LIBCHROME), TEST_PURPOSE'
            ])

    def test_re_compile(self):
        os.chdir(self.repo_dir.name)
        self.assertEqual(
            check_libchrome_utils.check(self.build_environ([BAD_FILE]),
                                        check_libchrome_utils.BAD_KEYWORDS), [])

