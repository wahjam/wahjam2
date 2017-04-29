#include "qt-solutions/qtwinmigrate/src/qwinwidget.h"

class TestWidget : public QWinWidget
{
    Q_OBJECT

public:
    TestWidget(HWND hParentWnd);
    ~TestWidget();

private slots:
    void muteToggled(bool enable);
};
