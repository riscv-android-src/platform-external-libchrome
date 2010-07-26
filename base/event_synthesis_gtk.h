// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file declares routines for creating fake GDK events (at the moment,
// only keyboard events). This is useful for a variety of testing purposes.
// NOTE: This should not be used outside of testing.

#ifndef BASE_EVENT_SYNTHESIS_GTK_
#define BASE_EVENT_SYNTHESIS_GTK_
#pragma once

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <vector>

#include "base/keyboard_codes.h"

namespace base {

// Creates and returns a key event. Passes ownership to the caller.
GdkEvent* SynthesizeKeyEvent(GdkWindow* event_window,
                             bool press,
                             guint gdk_key,
                             guint state);

// Creates the proper sequence of key events for a key press + release.
// Ownership of the events in the vector is passed to the caller.
void SynthesizeKeyPressEvents(
    GdkWindow* window,
    base::KeyboardCode key,
    bool control, bool shift, bool alt,
    std::vector<GdkEvent*>* events);

}  // namespace base

#endif  // BASE_EVENT_SYNTHESIS_GTK_
