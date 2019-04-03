//
//  util.hpp
//  earth_server
//
//  Created by 郭海 on 2019/4/1.
//

#ifndef util_h
#define util_h

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string>


/**
 字符串转换为双精度浮点数

 @param str 输入字符串10进制
 @param out 输出结果
 @return 是否成功
 */
bool safe_strtod(const char *str, double *out);

/**
 字符串转换为无符号长整型

 @param str <#str description#>
 @param out <#out description#>
 @return <#return value description#>
 */
bool safe_strtoul(const char *str, uint32_t *out);
#endif /* util_hpp */
