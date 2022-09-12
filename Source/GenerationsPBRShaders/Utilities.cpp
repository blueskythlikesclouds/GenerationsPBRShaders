#include "Utilities.h"

size_t Utilities::computeMipLevels(size_t width, size_t height)
{
    size_t mipLevels = 1;

    while (height > 1 || width > 1)
    {
        if (height > 1)
            height >>= 1;

        if (width > 1)
            width >>= 1;

        ++mipLevels;
    }

    return mipLevels;
}
