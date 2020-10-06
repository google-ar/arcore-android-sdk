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

/** Container class holding identifying information for an Anchor to be resolved. */
class AnchorItem {

  private final String anchorId;
  private final String anchorName;
  private final long minutesSinceCreation;
  private boolean selected;

  public AnchorItem(String anchorId, String anchorName, long minutesSinceCreation) {
    this.anchorId = anchorId;
    this.anchorName = anchorName;
    this.minutesSinceCreation = minutesSinceCreation;
    this.selected = false;
  }

  public String getAnchorName() {
    return anchorName;
  }

  public String getAnchorId() {
    return anchorId;
  }

  public String getMinutesSinceCreation() {
    if (minutesSinceCreation < 60) {
      return minutesSinceCreation + "m ago";
    } else {
      return (int) minutesSinceCreation / 60 + "hr ago";
    }
  }

  public boolean isSelected() {
    return selected;
  }

  public void setSelected(boolean selected) {
    this.selected = selected;
  }
}
