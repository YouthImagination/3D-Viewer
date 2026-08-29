// Single compilation unit for STB implementations and global AppLog
// This prevents duplicate symbol errors from multiple #define STB_*_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "logger.h"

// Global AppLog instance used by LOG_* macros
AppLog appLog;
