// image.cpp - GUIImage object

#include "gui/image.hpp"

GUIImage::GUIImage(xml_node<>* node) : GUIObject(node)
{
    mImage = nullptr;
    mHighlightImage = nullptr;
    isHighlighted = false;

    if (!node) {
        return;
    }

    mImage = LoadAttrImage(FindNode(node, "image"), "resource");
    mHighlightImage = LoadAttrImage(FindNode(node, "image"), "highlightresource");

    // Load the placement
    LoadPlacement(FindNode(node, "placement"), &mRenderX, &mRenderY, nullptr, nullptr, &mPlacement);

    if (mImage && mImage->GetResource()) {
        mRenderW = mImage->GetWidth();
        mRenderH = mImage->GetHeight();

        // Adjust for placement
        if (mPlacement != TOP_LEFT && mPlacement != BOTTOM_LEFT) {
            if (mPlacement == CENTER) {
                mRenderX -= (mRenderW / 2);
            } else {
                mRenderX -= mRenderW;
            }
        }
        if (mPlacement != TOP_LEFT && mPlacement != TOP_RIGHT) {
            if (mPlacement == CENTER) {
                mRenderY -= (mRenderH / 2);
            } else {
                mRenderY -= mRenderH;
            }
        }
        SetPlacement(TOP_LEFT);
    }
}

int GUIImage::Render()
{
    if (!isConditionTrue()) {
        return 0;
    }

    if (isHighlighted && mHighlightImage && mHighlightImage->GetResource()) {
        gr_blit(mHighlightImage->GetResource(), 0, 0, mRenderW, mRenderH, mRenderX, mRenderY);
        return 0;
    } else if (!mImage || !mImage->GetResource()) {
        return -1;
    }

    gr_blit(mImage->GetResource(), 0, 0, mRenderW, mRenderH, mRenderX, mRenderY);
    return 0;
}

int GUIImage::SetRenderPos(int x, int y, int w, int h)
{
    if (w || h) {
        return -1;
    }

    mRenderX = x;
    mRenderY = y;
    return 0;
}
