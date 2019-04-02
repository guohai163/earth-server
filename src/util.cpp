//
//  util.cpp
//  earth_server
//
//  Created by 郭海 on 2019/4/1.
//

#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#define xisspace(c) isspace((unsigned char)c)


bool safe_strtod(const char *str, double *out) {
    printf("double %s", str);
    assert(out != NULL);
    char *endptr;
    *out = 0;
    double d = strtod(str, &endptr);
    
    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = d;
        return true;
    }
    return false;
}

bool safe_strtoul(const char *str, uint32_t *out) {
    unsigned long l =0;
    char *endptr = NULL;
    *out = 0;
    errno = 0;
    
    l = strtoul(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }
    return false;
}
