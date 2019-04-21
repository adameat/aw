#pragma once

#ifndef VERIFY
    #define VERIFY(x) while (!(x)) { AW::Reset(#x); }
#endif