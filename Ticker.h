#pragma once

#include <QTimer>

class Ticker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    Ticker();
    int value() const;
    void setValue(int n);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void tick(int n);
    void valueChanged();

private slots:
    void onTimeout();

private:
    QTimer timer_;
    int value_;
};
