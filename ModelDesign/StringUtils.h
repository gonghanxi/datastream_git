#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <string>
#include <cstring>
#include <vector>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <string.h>
#endif

namespace StringUtils {

    // 安全的C字符串复制
    inline bool strcpy_safe(char* dest, size_t destSize, const char* src) {
        if (!dest || destSize == 0) return false;
        if (!src) {
            dest[0] = '\0';
            return true;
        }

    #ifdef _WIN32
        return strcpy_s(dest, destSize, src) == 0;
    #else
        size_t srcLen = strlen(src);
        if (srcLen >= destSize) {
            if (destSize > 0) {
                strncpy(dest, src, destSize - 1);
                dest[destSize - 1] = '\0';
            }
            return false;  // 截断发生
        } else {
            strcpy(dest, src);
            return true;
        }
    #endif
    }

    // 安全的字符串连接
    inline bool strcat_safe(char* dest, size_t destSize, const char* src) {
        if (!dest || destSize == 0 || !src) return false;

        size_t currentLen = strlen(dest);
        size_t srcLen = strlen(src);

        if (currentLen + srcLen >= destSize) {
            // 空间不足
            return false;
        }

    #ifdef _WIN32
        return strcat_s(dest, destSize, src) == 0;
    #else
        strcat(dest, src);
        return true;
    #endif
    }

    // 转换为std::string
    inline std::string toString(const char* str, size_t maxLen = 0) {
        if (!str) return std::string();
        if (maxLen > 0) {
            return std::string(str, strnlen(str, maxLen));
        }
        return std::string(str);
    }

    // 格式化字符串（跨平台sprintf的安全版本）
    template<typename... Args>
    std::string format(const char* fmt, Args... args) {
        int size = snprintf(nullptr, 0, fmt, args...);
        if (size <= 0) return std::string();

        std::vector<char> buffer(size + 1);
        snprintf(buffer.data(), buffer.size(), fmt, args...);
        return std::string(buffer.data());
    }

} // namespace StringUtils
#endif // STRINGUTILS_H
