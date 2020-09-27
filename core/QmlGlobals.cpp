// SPDX-License-Identifier: Apache-2.0
#include <QQmlEngine>
#include "QmlGlobals.h"
#include "SessionListModel.h"

QmlGlobals::QmlGlobals(QObject *parent)
    : QObject(parent)
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
        return new QmlGlobals;
    });
}
