// fill.cpp - GUIFill object

#include "gui/fill.hpp"

#include "mblog/logging.h"

GUIFill::GUIFill(xml_node<>* node) : GUIObject(node)
{
    bool has_color = false;
    mColor = LoadAttrColor(node, "color", &has_color);
    if (!has_color) {
        LOGE("No color specified for fill");
        return;
    }

    // Load the placement
    LoadPlacement(FindNode(node, "placement"), &mRenderX, &mRenderY, &mRenderW, &mRenderH);

    return;
}

int GUIFill::Render()
{
    if (!isConditionTrue()) {
        return 0;
    }

    gr_color(mColor.red, mColor.green, mColor.blue, mColor.alpha);
    gr_fill(mRenderX, mRenderY, mRenderW, mRenderH);
    return 0;
}

