#include <stdio.h>
#include <QPushButton>
#include "TestWidget.h"

TestWidget::TestWidget(HWND hParentWnd)
    : QWinWidget(hParentWnd)
{
    QPushButton *mute = new QPushButton("Mute", this);
    mute->setCheckable(true);
    connect(mute, SIGNAL(toggled(bool)),
            this, SLOT(muteToggled(bool)));
}

TestWidget::~TestWidget()
{
}

void TestWidget::muteToggled(bool enable)
{
    fprintf(stderr, "%s %d\n", __func__, enable);
}
