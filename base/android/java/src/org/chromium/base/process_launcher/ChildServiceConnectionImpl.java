// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.process_launcher;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;

import org.chromium.base.TraceEvent;
import org.chromium.base.compat.ApiHelperForQ;

import java.util.concurrent.Executor;

/** Implementation of ChildServiceConnection that does connect to a service. */
/* package */ class ChildServiceConnectionImpl
        implements ChildServiceConnection, ServiceConnection {
    private final Context mContext;
    private final Intent mBindIntent;
    private final int mBindFlags;
    private final Handler mHandler;
    private final Executor mExecutor;
    private final ChildServiceConnectionDelegate mDelegate;
    private final String mInstanceName;
    private boolean mBound;

    /* package */ ChildServiceConnectionImpl(Context context, Intent bindIntent, int bindFlags,
            Handler handler, Executor executor, ChildServiceConnectionDelegate delegate,
            String instanceName) {
        mContext = context;
        mBindIntent = bindIntent;
        mBindFlags = bindFlags;
        mHandler = handler;
        mExecutor = executor;
        mDelegate = delegate;
        mInstanceName = instanceName;
    }

    @Override
    public boolean bindServiceConnection() {
        try {
            TraceEvent.begin("ChildServiceConnectionImpl.bindServiceConnection");
            mBound = BindService.doBindService(
                    mContext, mBindIntent, this, mBindFlags, mHandler, mExecutor, mInstanceName);
        } finally {
            TraceEvent.end("ChildServiceConnectionImpl.bindServiceConnection");
        }
        return mBound;
    }

    @Override
    public void unbindServiceConnection() {
        if (mBound) {
            mContext.unbindService(this);
            mBound = false;
        }
    }

    @Override
    public boolean isBound() {
        return mBound;
    }

    @Override
    public void updateGroupImportance(int group, int importanceInGroup) {
        assert isBound();
        if (BindService.supportVariableConnections()) {
            ApiHelperForQ.updateServiceGroup(mContext, this, group, importanceInGroup);
            BindService.doBindService(
                    mContext, mBindIntent, this, mBindFlags, mHandler, mExecutor, mInstanceName);
        }
    }

    @Override
    public void onServiceConnected(ComponentName className, final IBinder service) {
        mDelegate.onServiceConnected(service);
    }

    // Called on the main thread to notify that the child service did not disconnect gracefully.
    @Override
    public void onServiceDisconnected(ComponentName className) {
        mDelegate.onServiceDisconnected();
    }
}
