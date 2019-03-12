#pragma once

#define VERIFY(x)                     \
    do                                \
    {                                 \
        if (!(x))                     \
            *((int *)0xfffffffc) = 1; \
    } while (false);
