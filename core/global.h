// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QString>

extern QString logFilePath;

void installMessageHandler();
void globalInit();
void globalCleanup();
void registerQmlTypes();
