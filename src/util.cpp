//
//  util.cpp
//  earth_server
//
//  Created by 郭海 on 2019/4/1.
//  website http://earth.guohai.org

#include "util.h"


#define xisspace(c) isspace((unsigned char)c)


/**
 字符串转换为双精度类型

 @param str <#str description#>
 @param out <#out description#>
 @return <#return value description#>
 */
bool safe_strtod(const char *str, double *out) {

    assert(out != NULL);
    errno = 0;
    char *endptr;
    *out = 0;
    double d = strtod(str, &endptr);
    
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }
    
    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = d;
        return true;
    }
    return false;
}

/**
 安全的将字符串转换为无符号整形

 @param str 输入的字符串
 @param out 输出的f结果
 @return 转换是否成功
 */
bool safe_strtoul(const char *str, uint32_t *out) {
    unsigned long l =0;
    char *endptr = NULL;
    assert(out);
    assert(str);
    *out = 0;
    errno = 0;
    
    l = strtoul(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }
    
    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long)l<0) {
            if (strchr(str, '-') != NULL) {
                return false;
            }
        }
        *out = l;
        return true;
    }
    return false;
}
