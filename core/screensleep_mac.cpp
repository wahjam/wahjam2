// SPDX-License-Identifier: Apache-2.0
/* Prevent screen from going to sleep. See
 * https://developer.apple.com/library/content/qa/qa1340/_index.html
 */

#include <IOKit/pwr_mgt/IOPMLib.h>

static CFStringRef activityName = CFSTR("Jam Session");
static IOPMAssertionID assertionID;

void screenPreventSleep()
{
  if (assertionID != kIOPMNullAssertionID) {
    return;
  }

  IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                              kIOPMAssertionLevelOn,
                              activityName,
                              &assertionID);
}

void screenAllowSleep()
{
  if (assertionID == kIOPMNullAssertionID) {
    return;
  }

  IOPMAssertionRelease(assertionID);
  assertionID = kIOPMNullAssertionID;
}
