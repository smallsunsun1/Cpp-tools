//
// Created by 孙嘉禾 on 2019/12/26.
//

#ifndef TOOLS_MACROS_H
#define TOOLS_MACROS_H

#define DISALLOW_COPY_AND_ASSIGN(Type) \
Type(const Type&) = delete; \
Type(Type &) = delete; \
Type& operator=(const Type&) = delete



#endif //TOOLS_MACROS_H
