/*
 * Copyright 2023 Google LLC
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

package com.google.ar.core.examples.java.hellosemantics;

import android.graphics.Bitmap;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.recyclerview.widget.RecyclerView;
import com.google.ar.core.SemanticLabel;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Semantic label adapter used for populating the label legend and update the label fraction value.
 */
public class SemanticLabelAdapter extends RecyclerView.Adapter<SemanticLabelAdapter.ViewHolder> {
  private static final String TAG = SemanticLabelAdapter.class.getSimpleName();

  private RecyclerView recyclerView;
  private List<SemanticLabelInfo> labels;

  private static class SemanticLabelInfo {
    public int color;
    public String name;
    public float fraction;

    public SemanticLabelInfo(int color, String name) {
      this.color = color;
      this.name = name;
      this.fraction = 0.0f;
    }
  }

  static class ViewHolder extends RecyclerView.ViewHolder {
    private final View color;
    private final TextView name;
    private final TextView fraction;

    private ViewHolder(View entry) {
      super(entry);
      color = entry.findViewById(R.id.color);
      name = (TextView) entry.findViewById(R.id.name);
      fraction = (TextView) entry.findViewById(R.id.fraction);
    }

    public View getColor() {
      return color;
    }

    public TextView getName() {
      return name;
    }

    public TextView getFraction() {
      return fraction;
    }
  }

  @Override
  public void onAttachedToRecyclerView(RecyclerView recyclerView) {
    super.onAttachedToRecyclerView(recyclerView);
    this.recyclerView = recyclerView;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int viewType) {
    View entry =
        LayoutInflater.from(viewGroup.getContext())
            .inflate(R.layout.semantics_color_item, viewGroup, false);
    ViewHolder viewHolder = new ViewHolder(entry);
    return viewHolder;
  }

  @Override
  public void onBindViewHolder(ViewHolder viewHolder, final int position) {
    if (labels == null || position >= labels.size()) {
      return;
    }

    SemanticLabelInfo labelInfo = labels.get(position);

    viewHolder.getColor().setBackgroundColor(labelInfo.color);
    viewHolder.getName().setText(labelInfo.name);
    viewHolder.getFraction().setText(getFractionText(labelInfo.fraction));
  }

  @Override
  public int getItemCount() {
    return labels.size();
  }

  private String getFractionText(float fraction) {
    return fraction > 0.0f
        ? String.format(Locale.getDefault(), " %.1f%%", (fraction * 100.0f))
        : " ---";
  }

  /**
   * Updates the fraction for the given label.
   *
   * @param label the semantics label to update
   * @param fraction the new fraction value.
   */
  public void updateLabelFraction(SemanticLabel label, float fraction) {
    int labelNumber = label.ordinal();
    if (labelNumber >= getItemCount()) {
      Log.w(TAG, "Aborting fraction update due to invalid label " + label);
      return;
    }
    ViewHolder viewHolder = (ViewHolder) recyclerView.findViewHolderForAdapterPosition(labelNumber);
    if (viewHolder != null) {
      viewHolder.getFraction().setText(getFractionText(fraction));
    }
  }

  /**
   * Creates the list of colors and names for the existing semantic labels.
   *
   * @param semanticsColorMap the semantic color map.
   */
  public void setupLabels(Bitmap semanticsColorMap) {
    if (semanticsColorMap == null) {
      Log.e(TAG, "Failed to create label list.");
    } else {
      labels = new ArrayList<>();
      for (SemanticLabel label : SemanticLabel.values()) {
        if (label.ordinal() >= semanticsColorMap.getWidth()) {
          Log.w(TAG, "Semantic label " + label.name() + "doesn't have a color map correspondence.");
          continue;
        }
        labels.add(
            new SemanticLabelInfo(semanticsColorMap.getPixel(label.ordinal(), 0), label.name()));
      }
    }
  }
}
