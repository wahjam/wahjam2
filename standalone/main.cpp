// SPDX-License-Identifier: Apache-2.0
#include <QGuiApplication>
#include <QQmlError>
#include <QQuickView>

#include "core/global.h"

// TODO AudioProcessor lifecycle and periodic tick
// TODO PortAudio audio thread that calls processor.process()

static void showViewErrors(QQuickView *view)
{
    for (const QQmlError &error : view->errors()) {
        qDebug("QML error: %s", error.toString().toUtf8().constData());
    }
}

int main(int argc, char **argv)
{
    installMessageHandler();

    QGuiApplication app(argc, argv);

    globalInit();
    registerQmlTypes();

    QQuickView *view = new QQuickView{QUrl{"qrc:/qml/application.qml"}};
    QObject::connect(view, &QQuickView::statusChanged,
        [=] (QQuickView::Status) { showViewErrors(view); });
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->show();

    int rc = app.exec();
    globalCleanup();
    return rc;
}
