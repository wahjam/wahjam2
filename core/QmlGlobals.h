// SPDX-License-Identifier: Apache-2.0
#include "AppView.h"
#include "JamApiManager.h"
#include "JamSession.h"

/*
 * QmlGlobals is the singleton object that QML/JavaScript code uses to access
 * the C++ APIs.
 */
class QmlGlobals : public QObject
{
    Q_OBJECT
    Q_PROPERTY(JamApiManager *apiManager READ apiManager NOTIFY apiManagerChanged)

    // The format (standalone, VST plugin, etc)
    Q_PROPERTY(QString format READ format)

    Q_PROPERTY(JamSession *session READ session NOTIFY sessionChanged)

public:
    QmlGlobals(AppView *appView, const QString &format, QObject *parent = nullptr);

    JamApiManager *apiManager()
    {
        return &apiManager_;
    }

    QString format() const
    {
        return format_;
    }

    JamSession *session()
    {
        return &session_;
    }

    // Register C++ classes with QML engine
    static void registerQmlTypes();

signals:
    void apiManagerChanged();
    void sessionChanged();

private:
    QString format_;
    JamApiManager apiManager_;
    JamSession session_;
};
