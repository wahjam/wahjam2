// SPDX-License-Identifier: Apache-2.0
/* Prevent screen from going to sleep */

#include <windows.h>

void screenPreventSleep()
{
  SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
}

void screenAllowSleep()
{
  SetThreadExecutionState(ES_CONTINUOUS);
}
