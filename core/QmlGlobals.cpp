// SPDX-License-Identifier: Apache-2.0
#include <QQmlEngine>
#include "QmlGlobals.h"
#include "SessionListModel.h"

QmlGlobals::QmlGlobals(AppView *appView, const QString &format, QObject *parent)
    : QObject(parent), format_{format}, session_{appView}
{
}

void QmlGlobals::registerQmlTypes()
{
    // TODO how to unregister types?
    qmlRegisterType<SessionListModel>("org.wahjam.client", 1, 0, "SessionListModel");
    qmlRegisterUncreatableType<JamApiManager>("org.wahjam.client", 1, 0, "JamApiManager",
                                              "Use Client.apiManager singleton instead");
    qmlRegisterUncreatableType<JamSession>("org.wahjam.client", 1, 0, "JamSession",
                                           "Use Client.session singleton instead");
    qmlRegisterSingletonType<QmlGlobals>("org.wahjam.client", 1, 0, "Client",
                                         [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        // Assume this singleton is only created from AppView's QQmlEngine.
        // Find AppView by searching the QObject parent chain for the
        // QQmlEngine. Even in a plugin environment where other code is using
        // Qt this should work because the singleton creation function is only
        // called from our QML code. Note that a global AppView variable cannot
        // be used since multiple plugin instances share globals.
        for (QObject *obj = engine; obj; obj = obj->parent()) {
            auto appView = qobject_cast<AppView *>(obj);
            if (appView) {
                QmlGlobals *qmlGlobals = appView->qmlGlobals();

                // appView owns qmlGlobals, so don't let QQmlEngine take ownership
                QQmlEngine::setObjectOwnership(qmlGlobals, QQmlEngine::CppOwnership);

                return qmlGlobals;
            }
        }

        qFatal("Cannot find AppView needed for QmlGlobals instantiation");
    });
}
