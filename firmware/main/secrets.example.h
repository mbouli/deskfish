/* Copy this file to secrets.h (which git ignores) and fill in your Wi-Fi.
 * Without a secrets.h the firmware still builds: it just never learns the
 * real time, and assumes the moment it was built. */
#pragma once

#define WIFI_SSID "your network name"
#define WIFI_PASSWORD "your password"
