//
//  util.cpp
//  earth_server
//
//  Created by 郭海 on 2019/4/1.
//

#include "util.h"
#include <assert.h>
#include <ctype.h>

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

