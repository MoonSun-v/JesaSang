#pragma once
#include "../Manager/ComponentFactory.h"

template<typename T>
struct AutoComponentRegister
{
    explicit AutoComponentRegister(const char* name)
    {
        ComponentFactory::Instance().Register<T>(name);
    }
};

#define REGISTER_COMPONENT(T) \
    static AutoComponentRegister<T> _auto_reg_##T(#T);