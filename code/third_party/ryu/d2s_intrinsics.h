// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// Returns true if value is divisible by 2^p.
static inline bool
multipleOfPowerOf2(const uint64_t value, const uint32_t p)
{
    // assert(value != 0);
    // assert(p < 64);
    //  __builtin_ctzll doesn't appear to be faster here.
    return (value & ((1ull << p) - 1)) == 0;
}
