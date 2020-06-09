// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TIMER_ELAPSED_TIMER_H_
#define BASE_TIMER_ELAPSED_TIMER_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/time/time.h"

namespace base {

// A simple wrapper around TimeTicks::Now().
class BASE_EXPORT ElapsedTimer {
 public:
  ElapsedTimer();
  ElapsedTimer(ElapsedTimer&& other);

  void operator=(ElapsedTimer&& other);

  // Returns the time elapsed since object construction.
  TimeDelta Elapsed() const;

  // Returns the timestamp of the creation of this timer.
  TimeTicks Begin() const { return begin_; }

 private:
  TimeTicks begin_;

  DISALLOW_COPY_AND_ASSIGN(ElapsedTimer);
};

// A simple wrapper around ThreadTicks::Now().
class BASE_EXPORT ElapsedThreadTimer {
 public:
  ElapsedThreadTimer();

  // Returns the ThreadTicks time elapsed since object construction.
  // Only valid if |is_supported()| returns true, otherwise returns TimeDelta().
  TimeDelta Elapsed() const;

  bool is_supported() const { return is_supported_; }

 private:
  const bool is_supported_;
  const ThreadTicks begin_;

  DISALLOW_COPY_AND_ASSIGN(ElapsedThreadTimer);
};

}  // namespace base

#endif  // BASE_TIMER_ELAPSED_TIMER_H_
