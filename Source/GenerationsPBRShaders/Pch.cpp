#include "Pch.h"

#if !_DEBUG

namespace boost
{
    void throw_exception(std::exception const& e)
    {
        __debugbreak();
    }
}

namespace hl
{
    std::exception make_exception(error_type err) { __debugbreak(); return std::exception(); }
    std::exception make_exception(error_type err, const char* what_arg) { __debugbreak(); return std::exception(); }
}

#endif

bool globalUsePBR;