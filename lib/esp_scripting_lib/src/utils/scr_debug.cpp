
#include "scr_debug.h"

static std::string error_msg_buffer;

void WRITE(char *buffer)
{
    Serial.write(buffer);
}

size_t PRINTF(const char *format, ...)
{
    int len = 0;
    char loc_buf[64];
    char *temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return 0;
    }
    if(len >= sizeof(loc_buf)) {
        temp = (char*) malloc(len + 1);
        if(temp == NULL) {
            va_end(arg);
            return 0;
        }
        len = vsnprintf(temp, len + 1, format, arg);
    }
    va_end(arg);
    WRITE(temp);
    if(temp != loc_buf){
        free(temp);
    }
    return len;
}

size_t PRINT_ERROR(const char *format, ...)
{
    int len = 0;
    char buffer[256];
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(buffer, sizeof(buffer), format, copy);
    va_end(copy);
    error_msg_buffer += buffer;
    va_end(arg);
    return len;
}

std::string DEBUG_GET_ERROR_MESSAGE() {
    return error_msg_buffer;
}

void debug_clear_error_message() {
    error_msg_buffer.clear();
}