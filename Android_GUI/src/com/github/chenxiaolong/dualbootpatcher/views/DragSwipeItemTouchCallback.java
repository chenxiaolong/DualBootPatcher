/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Based on Apache2-licensed code from https://github.com/iPaulPro/Android-ItemTouchHelper-Demo
 */

package com.github.chenxiaolong.dualbootpatcher.views;

import android.graphics.Canvas;
import android.support.annotation.NonNull;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.support.v7.widget.helper.ItemTouchHelper;

public class DragSwipeItemTouchCallback extends ItemTouchHelper.Callback {
    private OnItemMovedOrDismissedListener mListener;
    private boolean mDragEnabled = true;
    private boolean mSwipeEnabled = true;

    public DragSwipeItemTouchCallback(@NonNull OnItemMovedOrDismissedListener listener) {
        mListener = listener;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getMovementFlags(RecyclerView recyclerView, ViewHolder viewHolder) {
        int dragFlags;
        int swipeFlags;

        if (recyclerView.getLayoutManager() instanceof GridLayoutManager) {
            // Allow dragging in any direction for a grid layout
            dragFlags = ItemTouchHelper.UP | ItemTouchHelper.DOWN |
                    ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT;
            // But don't allow swiping
            swipeFlags = 0;
        } else {
            // Allow up/down dragging for vertically oriented linear layouts
            dragFlags = ItemTouchHelper.UP | ItemTouchHelper.DOWN;
            // Allow left/right (start/end) swiping
            swipeFlags = ItemTouchHelper.START | ItemTouchHelper.END;
        }

        return makeMovementFlags(dragFlags, swipeFlags);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean onMove(RecyclerView recyclerView, ViewHolder viewHolder, ViewHolder target) {
        // Don't allow mixing types
        if (viewHolder.getItemViewType() != target.getItemViewType()) {
            return false;
        }

        mListener.onItemMoved(viewHolder.getAdapterPosition(), target.getAdapterPosition());
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onSwiped(ViewHolder viewHolder, int direction) {
        mListener.onItemDismissed(viewHolder.getAdapterPosition());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onChildDraw(Canvas c, RecyclerView recyclerView, ViewHolder viewHolder, float dX,
                            float dY, int actionState, boolean isCurrentlyActive) {
        // Fade out when swiping the item
        if (actionState == ItemTouchHelper.ACTION_STATE_SWIPE) {
            final float alpha = 1.0f - Math.abs(dX) / (float) viewHolder.itemView.getWidth();
            viewHolder.itemView.setAlpha(alpha);
            viewHolder.itemView.setTranslationX(dX);
        } else {
            super.onChildDraw(c, recyclerView, viewHolder, dX, dY, actionState, isCurrentlyActive);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void clearView(RecyclerView recyclerView, ViewHolder viewHolder) {
        super.clearView(recyclerView, viewHolder);

        viewHolder.itemView.setAlpha(1.0f);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isLongPressDragEnabled() {
        return mDragEnabled;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isItemViewSwipeEnabled() {
        return mSwipeEnabled;
    }

    public void setLongPressDragEnabled(boolean enabled) {
        mDragEnabled = enabled;
    }

    public void setItemViewSwipeEnabled(boolean enabled) {
        mSwipeEnabled = enabled;
    }

    public interface OnItemMovedOrDismissedListener {
        void onItemMoved(int fromPosition, int toPosition);

        void onItemDismissed(int position);
    }
}
