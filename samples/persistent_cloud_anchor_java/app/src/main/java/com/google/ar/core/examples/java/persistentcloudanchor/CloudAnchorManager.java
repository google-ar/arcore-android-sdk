/*
 * Copyright 2020 Google LLC
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

package com.google.ar.core.examples.java.persistentcloudanchor;

import com.google.ar.core.Anchor;
import com.google.ar.core.Anchor.CloudAnchorState;
import com.google.ar.core.HostCloudAnchorFuture;
import com.google.ar.core.ResolveCloudAnchorFuture;
import com.google.ar.core.Session;
import com.google.common.base.Preconditions;

/**
 * A helper class to handle all the Cloud Anchors logic, and add a callback-like mechanism on top of
 * the existing ARCore API.
 */
class CloudAnchorManager {

  /** Listener for the results of a host operation. */
  interface CloudAnchorHostListener {

    /** This method is invoked when the results of a Cloud Anchor operation are available. */
    void onComplete(String cloudAnchorId, CloudAnchorState cloudAnchorState);
  }

  /** Listener for the results of a resolve operation. */
  interface CloudAnchorResolveListener {

    /** This method is invoked when the results of a Cloud Anchor operation are available. */
    void onComplete(String cloudAnchorId, Anchor anchor, CloudAnchorState cloudAnchorState);
  }

  private Session session;

  CloudAnchorManager(Session session) {
    this.session = Preconditions.checkNotNull(session);
  }

  /** Hosts an anchor. The {@code listener} will be invoked when the results are available. */
  synchronized void hostCloudAnchor(Anchor anchor, CloudAnchorHostListener listener) {
    Preconditions.checkNotNull(listener, "The listener cannot be null.");
    // Creating a Cloud Anchor with lifetime  = 1 day. This is configurable up to 365 days.
    HostCloudAnchorFuture unused =
        session.hostCloudAnchorAsync(anchor, /* ttlDays= */ 1, listener::onComplete);
  }

  /** Resolves an anchor. The {@code listener} will be invoked when the results are available. */
  synchronized void resolveCloudAnchor(final String anchorId, CloudAnchorResolveListener listener) {
    Preconditions.checkNotNull(listener, "The listener cannot be null.");
    ResolveCloudAnchorFuture unused =
        session.resolveCloudAnchorAsync(
            anchorId,
            (anchor, cloudAnchorState) -> {
              listener.onComplete(anchorId, anchor, cloudAnchorState);
            });
  }
}
