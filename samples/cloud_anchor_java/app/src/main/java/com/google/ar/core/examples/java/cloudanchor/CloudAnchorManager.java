/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ar.core.examples.java.cloudanchor;

import android.os.SystemClock;
import android.util.Pair;
import androidx.annotation.Nullable;
import com.google.ar.core.Anchor;
import com.google.ar.core.Anchor.CloudAnchorState;
import com.google.ar.core.FutureState;
import com.google.ar.core.HostCloudAnchorFuture;
import com.google.ar.core.ResolveCloudAnchorFuture;
import com.google.ar.core.Session;
import com.google.common.base.Preconditions;
import java.util.ArrayList;
import java.util.Iterator;

/**
 * A helper class to handle all the Cloud Anchors logic, and add a callback-like mechanism on top of
 * the existing ARCore API.
 */
class CloudAnchorManager {
  private static final String TAG =
      CloudAnchorActivity.class.getSimpleName() + "." + CloudAnchorManager.class.getSimpleName();
  private static final long DURATION_FOR_NO_RESOLVE_RESULT_MS = 10000;
  private long deadlineForMessageMillis;

  /** Listener for the results of a host operation. */
  interface CloudAnchorHostListener {

    /** This method is invoked when the results of a Cloud Anchor operation are available. */
    void onCloudTaskComplete(@Nullable String cloudAnchorId, CloudAnchorState cloudAnchorState);
  }

  /** Listener for the results of a resolve operation. */
  interface CloudAnchorResolveListener {

    /** This method is invoked when the results of a Cloud Anchor operation are available. */
    void onCloudTaskComplete(@Nullable Anchor anchor, CloudAnchorState cloudAnchorState);

    /** This method show the toast message. */
    void onShowResolveMessage();
  }

  @Nullable private Session session = null;

  /** The pending hosting operations. */
  private final ArrayList<Pair<HostCloudAnchorFuture, CloudAnchorHostListener>> hostTasks =
      new ArrayList<>();

  /** The pending resolving operations. */
  private final ArrayList<Pair<ResolveCloudAnchorFuture, CloudAnchorResolveListener>> resolveTasks =
      new ArrayList<>();

  /**
   * This method is used to set the session, since it might not be available when this object is
   * created.
   */
  synchronized void setSession(Session session) {
    this.session = session;
  }

  /**
   * This method hosts an anchor. The {@code listener} will be invoked when the results are
   * available.
   */
  synchronized void hostCloudAnchor(Anchor anchor, CloudAnchorHostListener listener) {
    Preconditions.checkNotNull(session, "The session cannot be null.");
    HostCloudAnchorFuture future = session.hostCloudAnchorAsync(anchor, 1, null);
    hostTasks.add(new Pair<>(future, listener));
  }

  /**
   * This method resolves an anchor. The {@code listener} will be invoked when the results are
   * available.
   */
  synchronized void resolveCloudAnchor(
      String anchorId, CloudAnchorResolveListener listener, long startTimeMillis) {
    Preconditions.checkNotNull(session, "The session cannot be null.");
    ResolveCloudAnchorFuture future = session.resolveCloudAnchorAsync(anchorId, null);
    resolveTasks.add(new Pair<>(future, listener));
    deadlineForMessageMillis = startTimeMillis + DURATION_FOR_NO_RESOLVE_RESULT_MS;
  }

  /** Should be called after a {@link Session#update()} call. */
  synchronized void onUpdate() {
    Preconditions.checkNotNull(session, "The session cannot be null.");
    Iterator<Pair<HostCloudAnchorFuture, CloudAnchorHostListener>> hostIter = hostTasks.iterator();
    while (hostIter.hasNext()) {
      Pair<HostCloudAnchorFuture, CloudAnchorHostListener> entry = hostIter.next();
      if (entry.first.getState() == FutureState.DONE) {
        CloudAnchorHostListener listener = entry.second;
        String cloudAnchorId = entry.first.getResultCloudAnchorId();
        CloudAnchorState cloudAnchorState = entry.first.getResultCloudAnchorState();
        listener.onCloudTaskComplete(cloudAnchorId, cloudAnchorState);
        hostIter.remove();
      }
    }

    Iterator<Pair<ResolveCloudAnchorFuture, CloudAnchorResolveListener>> resolveIter =
        resolveTasks.iterator();
    while (resolveIter.hasNext()) {
      Pair<ResolveCloudAnchorFuture, CloudAnchorResolveListener> entry = resolveIter.next();
      CloudAnchorResolveListener listener = entry.second;
      if (entry.first.getState() == FutureState.DONE) {
        Anchor anchor = entry.first.getResultAnchor();
        CloudAnchorState cloudAnchorState = entry.first.getResultCloudAnchorState();
        listener.onCloudTaskComplete(anchor, cloudAnchorState);
        resolveIter.remove();
      }
      if (deadlineForMessageMillis > 0 && SystemClock.uptimeMillis() > deadlineForMessageMillis) {
        listener.onShowResolveMessage();
        deadlineForMessageMillis = 0;
      }
    }
  }

  /** Used to clear any currently registered listeners, so they won't be called again. */
  synchronized void clearListeners() {
    hostTasks.clear();
    resolveTasks.clear();
    deadlineForMessageMillis = 0;
  }
}
