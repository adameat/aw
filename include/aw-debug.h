#pragma once

#ifndef VERIFY
    #define VERIFY(x) while (!(x) && *((int *)0xfffffffc) != 1);
#endif