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

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;
import androidx.annotation.Nullable;
import java.util.List;

/** Multi-select dropdown for selecting Anchors to resolve. */
public class MultiSelectItem extends ArrayAdapter<AnchorItem> {
  private LayoutInflater layoutInflater;
  private final List<AnchorItem> anchorsList;
  public Spinner spinner = null;
  private static final String TAG = MultiSelectItem.class.getSimpleName();
  private static final String CHECKED_BOX = "\u2611";
  private static final String UNCHECKED_BOX = "\u2610";
  private static final String SPACE = "    ";

  public MultiSelectItem(Context context, int resource, List<AnchorItem> objects, Spinner spinner) {
    super(context, resource, objects);
    this.anchorsList = objects;
    this.spinner = spinner;
    layoutInflater = LayoutInflater.from(context);
  }

  // Adjust for the blank initial selection item.
  @Override
  public int getCount() {
    return super.getCount() + 1;
  }

  @Override
  public View getDropDownView(int position, View view, ViewGroup parent) {
    return getCustomView(position, view, parent);
  }

  @Override
  public View getView(int position, View view, ViewGroup parent) {
    return getCustomView(position, view, parent);
  }

  static class ViewHolder {
    TextView anchorName;
    TextView creationTime;
  }

  // Creates a view if convertView is null, otherwise reuse cached view.
  public View getCustomView(final int position, @Nullable View convertView, ViewGroup parent) {
    final ViewHolder viewHolder;
    if (convertView == null) {
      viewHolder = new ViewHolder();
      layoutInflater =
          (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      convertView = layoutInflater.inflate(R.layout.anchor_item, null, false);
      viewHolder.anchorName = (TextView) convertView.findViewById(R.id.anchor_name);
      viewHolder.creationTime = (TextView) convertView.findViewById(R.id.creation_time);
      convertView.setTag(viewHolder);
    } else {
      viewHolder = (ViewHolder) convertView.getTag();
    }

    String text;
    int anchorPosition = position - 1;
    if (position == 0) {
      viewHolder.anchorName.setText("    Select Anchors");
      viewHolder.creationTime.setText("");
    } else {
      if (anchorsList.get(anchorPosition).isSelected()) {
        text = SPACE + CHECKED_BOX + SPACE + anchorsList.get(anchorPosition).getAnchorName();
      } else {
        text = SPACE + UNCHECKED_BOX + SPACE + anchorsList.get(anchorPosition).getAnchorName();
      }
      viewHolder.anchorName.setText(text);
      viewHolder.anchorName.setTag(anchorPosition);
      viewHolder.creationTime.setText(anchorsList.get(anchorPosition).getMinutesSinceCreation());
      viewHolder.anchorName.setOnClickListener(
          new View.OnClickListener() {
            @Override
            public void onClick(View v) {
              spinner.performClick();
              int getPosition = (Integer) v.getTag();
              anchorsList.get(getPosition).setSelected(!anchorsList.get(getPosition).isSelected());
              notifyDataSetChanged();
            }
          });
    }
    return convertView;
  }
}
