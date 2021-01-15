#!/usr/bin/env python3
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Utility to check if deprecated libchrome calls are introduced.
"""

from __future__ import print_function

import os
import re
import subprocess
import sys

# BAD_KEYWORDS are mapping from bad_keyword_in_regex to
# error_msg_if_match_found.
BAD_KEYWORDS = {
    b'DISALLOW_COPY_AND_ASSIGN':
    'Chromium agreed to return Google C++ style. Use deleted constructor in `public:` manually. See crbug/1010217',
    b'base::DeleteFile\(':
    '2-param base::DeleteFile will be deprecated. Use 1-param base::DeleteFile for non-recursive and base::DeletePathRecursively for recursive deletions',
    b'base::DeleteFileRecursively':
    'Use base::DeletePathRecursively instead',
    b'LOG_(VERBOSE|INFO|WARNING|ERROR|FATAL|NUM_SEVERITIES|DFATAL)':
    'Use LOGGING_* instead',
    b'\.(start|end)s_with\(':
    'BasicStringPiece::{starts,ends}_with will be deprecatd. Use base::{Starts,Ends}With instead',
    b'MessageLoopCurrent(|ForIO|ForUI)':
    'Use base::Current{,IO,UI}Thread instead with header "base/task/current_thread.h"',
}


def check(environ=os.environ, keywords=BAD_KEYWORDS):
    files = environ['PRESUBMIT_FILES'].split('\n')

    errors = []

    for f in files:
        if not (f.endswith('.h') or f.endswith('.cc')):
            continue
        diff = subprocess.check_output(
            ['git', 'show', '--oneline', environ['PRESUBMIT_COMMIT'], '--',
             f]).split(b'\n')
        for line in diff:
            if not line.startswith(b'+'):
                continue
            for bad_pattern, error_message in keywords.items():
                m = re.search(bad_pattern, line)
                if m:
                    errors.append('In File %s, found %s (pattern: %s), %s' %
                                  (f, m.group(0), bad_pattern.decode('ascii'),
                                   error_message))
                    break

    return errors


def main():
    errors = check()
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
