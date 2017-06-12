#include <QtGlobal>

#include "Ticker.h"

Ticker::Ticker()
    : value_(0)
{
    connect(&timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

int Ticker::value() const
{
    return value_;
}

void Ticker::setValue(int n)
{
    value_ = n;
    emit valueChanged();
}

void Ticker::start()
{
    timer_.start(1000 /* ms */);
}

void Ticker::stop()
{
    timer_.stop();
}

void Ticker::onTimeout()
{
    setValue(value_ + 1);
    emit tick(value_);
}
