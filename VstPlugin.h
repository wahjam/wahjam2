#pragma once

#include <QQuickView>

#include "aeffectx.h"

class VstPlugin : public QObject
{
    Q_OBJECT

public:
    struct MyRect {
        short top;
        short left;
        short bottom;
        short right;
    };

    AEffect aeffect;
    MyRect viewRect;
    static VstPlugin *fromAEffect(AEffect *aeffect);

    VstPlugin();
    ~VstPlugin();

    int editOpen(WId parentId);
    void editClose();
    void editIdle();

private:
    QQuickView *view;
};
