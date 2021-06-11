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
    # r890000 uprev
    b'DISALLOW_COPY_AND_ASSIGN':
    'Chromium agreed to return Google C++ style. Use deleted constructor in `public:` manually. See crbug/1010217',
    b'\.as_string\(\)':
    'base::BasicStringPiece::as_string is deprecated. Please use std::string() instead.',
    b'base::WriteFileDescriptor\([^(){}[\]]+,[^(){}[\]]+,[^(){}[\]]+\)':
    '3-param base::WriteFileDescriptor is deprecated. Please use 2-param variants instead. See crrev.com/c/2895444. Please ignore this warning if this is a false positive, sorry!',
    b'base::AppendToFile\([^(){}[\]]+,[^(){}[\]]+,[^(){}[\]]+\)':
    '3-param base::AppendToFile is deprecated. Please use 2-param variants instead. See crrev.com/c/2893183. Please ignore this warning if this is a false positive, sorry!',
    b'base::Align\(':
    'base::Align would be renamed as base::AlignUp.',
    # removal of deprecated base::Value APIs
    b'base::(Dictionary|List)Value':
    'base::{Dictionary,List}Value are deprecated. Please use dictionary/list-type base::Value.',
    # removal of deprecated base::Bind APIs
    b'base::(Bind\(|Closure|Callback|CancelableCallback|CancelableClosure)':
    'Deprecated base::Bind APIs. Please use the Once or Repeating variants. See crbug/714018.',
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
