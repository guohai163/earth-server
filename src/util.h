//
//  util.hpp
//  earth_server
//
//  Created by 郭海 on 2019/4/1.
//

#ifndef util_h
#define util_h

#include <stdio.h>

//!
/*!
 \param str 输入字符串
 \param out 输出的结果
 \return 成功与否
 */
bool safe_strtod(const char *str, double *out);
#endif /* util_hpp */
