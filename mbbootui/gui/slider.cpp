// slider.cpp - GUISlider object
// Pulled & ported from https://raw.github.com/agrabren/RecoverWin/master/gui/slider.cpp

#include "gui/slider.hpp"

#include "mblog/logging.h"

#include "data.hpp"
#include "variables.h"

GUISlider::GUISlider(xml_node<>* node) : GUIObject(node)
{
    xml_node<>* child;

    sAction = nullptr;
    sSliderLabel = nullptr;
    sSlider = nullptr;
    sSliderUsed = nullptr;
    sTouch = nullptr;
    sTouchW = 20;

    if (!node) {
        LOGE("GUISlider created without XML node");
        return;
    }

    // Load the resources
    child = FindNode(node, "resource");
    if (child) {
        sSlider = LoadAttrImage(child, "base");
        sSliderUsed = LoadAttrImage(child, "used");
        sTouch = LoadAttrImage(child, "touch");
    }

    // Load the text label
    sSliderLabel = new GUIText(node);
    if (sSliderLabel->Render() < 0) {
        delete sSliderLabel;
        sSliderLabel = nullptr;
    }

    // Load the placement
    Placement TextPlacement = CENTER;
    LoadPlacement(FindNode(node, "placement"),
                  &mRenderX, &mRenderY, &mRenderW, &mRenderH, &TextPlacement);

    mRenderW = sSlider->GetWidth();
    mRenderH = sSlider->GetHeight();
    if (TextPlacement == CENTER || TextPlacement == CENTER_X_ONLY) {
        mRenderX = mRenderX - (mRenderW / 2);
        if (TextPlacement == CENTER) {
            mRenderY = mRenderY - (mRenderH / 2);
        }
    }
    if (sSliderLabel) {
        int sTextX = mRenderX + (mRenderW / 2);
        int w, h;
        sSliderLabel->GetCurrentBounds(w, h);
        int sTextY = mRenderY + ((mRenderH - h) / 2);
        sSliderLabel->SetRenderPos(sTextX, sTextY);
        sSliderLabel->SetMaxWidth(mRenderW);
    }
    if (sTouch && sTouch->GetResource()) {
        sTouchW = sTouch->GetWidth();  // Width of the "touch image" that follows the touch (arrow)
        sTouchH = sTouch->GetHeight(); // Height of the "touch image" that follows the touch (arrow)
    }

    //LOGI("mRenderW: %i mTouchW: %i", mRenderW, mTouchW);
    mActionX = mRenderX;
    mActionY = mRenderY;
    mActionW = mRenderW;
    mActionH = mRenderH;

    sAction = new GUIAction(node);

    sCurTouchX = mRenderX;
    sUpdate = 1;
}

GUISlider::~GUISlider()
{
    delete sAction;
    delete sSliderLabel;
}

int GUISlider::Render()
{
    if (!isConditionTrue()) {
        return 0;
    }

    if (!sSlider || !sSlider->GetResource()) {
        return -1;
    }

    // Draw the slider
    gr_blit(sSlider->GetResource(), 0, 0, mRenderW, mRenderH, mRenderX, mRenderY);

    // Draw the used
    if (sSliderUsed && sSliderUsed->GetResource() && sCurTouchX > mRenderX) {
        gr_blit(sSliderUsed->GetResource(), 0, 0, sCurTouchX - mRenderX, mRenderH, mRenderX, mRenderY);
    }

    // Draw the touch icon
    if (sTouch && sTouch->GetResource()) {
        gr_blit(sTouch->GetResource(), 0, 0, sTouchW, sTouchH, sCurTouchX, (mRenderY + ((mRenderH - sTouchH) / 2)));
    }

    if (sSliderLabel) {
        int ret = sSliderLabel->Render();
        if (ret < 0) {
            return ret;
        }
    }

    sUpdate = 0;
    return 0;
}

int GUISlider::Update()
{
    if (!isConditionTrue()) {
        return 0;
    }

    if (sUpdate) {
        return 2;
    }
    return 0;
}

int GUISlider::NotifyTouch(TOUCH_STATE state, int x, int y)
{
    if (!isConditionTrue()) {
        return -1;
    }

    static bool dragging = false;

    switch (state) {
    case TOUCH_START:
        if (x >= mRenderX && x <= mRenderX + sTouchW &&
                y >= mRenderY && y <= mRenderY + mRenderH) {
            sCurTouchX = x - (sTouchW / 2);
            if (sCurTouchX < mRenderX) {
                sCurTouchX = mRenderX;
            }
            dragging = true;
        }
        break;

    case TOUCH_DRAG:
        if (!dragging) {
            return 0;
        }
        if (y < mRenderY - sTouchH || y > mRenderY + (sTouchH * 2)) {
            sCurTouchX = mRenderX;
            dragging = false;
            sUpdate = 1;
            break;
        }
        sCurTouchX = x - (sTouchW / 2);
        if (sCurTouchX < mRenderX) {
            sCurTouchX = mRenderX;
        }
        if (sCurTouchX > mRenderX + mRenderW - sTouchW) {
            sCurTouchX = mRenderX + mRenderW - sTouchW;
        }
        sUpdate = 1;
        break;

    case TOUCH_RELEASE:
        if (!dragging) {
            return 0;
        }

        if (sCurTouchX >= mRenderX + mRenderW - sTouchW) {
            DataManager::Vibrate(VAR_TW_BUTTON_VIBRATE);
            sAction->doActions();
        }

        sCurTouchX = mRenderX;
        dragging = false;
        sUpdate = 1;
    case TOUCH_REPEAT:
    case TOUCH_HOLD:
        break;
    }
    return 0;
}
