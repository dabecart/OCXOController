#include "Logs.h"

char lastLog[128] = "by @dabecart";

void logMessage(char* msg) {
    strncpy(lastLog, msg, sizeof(lastLog));

    // These logs could be saved on EEPROM (?).
}
