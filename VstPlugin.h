#pragma once

#include <QQuickView>
#include <QMutex>

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
    QMutex dispatcherMutex;

    static VstPlugin *fromAEffect(AEffect *aeffect);

    VstPlugin();
    ~VstPlugin();

public slots:
    int editOpen(void *ptrarg);
    void editClose();
    void editIdle();

private:
    QQuickView *view;
};
