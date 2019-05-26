/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.views

import android.graphics.Canvas
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder

class DragSwipeItemTouchCallback(private val listener: OnItemMovedOrDismissedListener)
        : ItemTouchHelper.Callback() {
    private var dragEnabled = true
    private var swipeEnabled = true

    /**
     * {@inheritDoc}
     */
    override fun getMovementFlags(recyclerView: RecyclerView, viewHolder: ViewHolder): Int {
        val dragFlags: Int
        val swipeFlags: Int

        if (recyclerView.layoutManager is GridLayoutManager) {
            // Allow dragging in any direction for a grid layout
            dragFlags = ItemTouchHelper.UP or ItemTouchHelper.DOWN or
                    ItemTouchHelper.LEFT or ItemTouchHelper.RIGHT
            // But don't allow swiping
            swipeFlags = 0
        } else {
            // Allow up/down dragging for vertically oriented linear layouts
            dragFlags = ItemTouchHelper.UP or ItemTouchHelper.DOWN
            // Allow left/right (start/end) swiping
            swipeFlags = ItemTouchHelper.START or ItemTouchHelper.END
        }

        return ItemTouchHelper.Callback.makeMovementFlags(dragFlags, swipeFlags)
    }

    /**
     * {@inheritDoc}
     */
    override fun onMove(recyclerView: RecyclerView, viewHolder: ViewHolder,
                        target: ViewHolder): Boolean {
        // Don't allow mixing types
        if (viewHolder.itemViewType != target.itemViewType) {
            return false
        }

        listener.onItemMoved(viewHolder.adapterPosition, target.adapterPosition)
        return true
    }

    /**
     * {@inheritDoc}
     */
    override fun onSwiped(viewHolder: ViewHolder, direction: Int) {
        listener.onItemDismissed(viewHolder.adapterPosition)
    }

    /**
     * {@inheritDoc}
     */
    override fun onChildDraw(c: Canvas, recyclerView: RecyclerView, viewHolder: ViewHolder,
                             dX: Float, dY: Float, actionState: Int, isCurrentlyActive: Boolean) {
        // Fade out when swiping the item
        if (actionState == ItemTouchHelper.ACTION_STATE_SWIPE) {
            val alpha = 1.0f - Math.abs(dX) / viewHolder.itemView.width.toFloat()
            viewHolder.itemView.alpha = alpha
            viewHolder.itemView.translationX = dX
        } else {
            super.onChildDraw(c, recyclerView, viewHolder, dX, dY, actionState, isCurrentlyActive)
        }
    }

    /**
     * {@inheritDoc}
     */
    override fun clearView(recyclerView: RecyclerView, viewHolder: ViewHolder) {
        super.clearView(recyclerView, viewHolder)

        viewHolder.itemView.alpha = 1.0f
    }

    /**
     * {@inheritDoc}
     */
    override fun isLongPressDragEnabled(): Boolean {
        return dragEnabled
    }

    /**
     * {@inheritDoc}
     */
    override fun isItemViewSwipeEnabled(): Boolean {
        return swipeEnabled
    }

    fun setLongPressDragEnabled(enabled: Boolean) {
        dragEnabled = enabled
    }

    fun setItemViewSwipeEnabled(enabled: Boolean) {
        swipeEnabled = enabled
    }

    interface OnItemMovedOrDismissedListener {
        fun onItemMoved(fromPosition: Int, toPosition: Int)

        fun onItemDismissed(position: Int)
    }
}