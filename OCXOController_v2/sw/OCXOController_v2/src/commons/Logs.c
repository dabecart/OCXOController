#include "Logs.h"

char lastLog[128];

void logMessage(char* msg) {
    strncpy(lastLog, msg, sizeof(lastLog)-1);

    // These logs could be saved on EEPROM (?).
}
