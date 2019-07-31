//
// Created by Shashank Kaldate on 5/28/19.
//

//#ifndef DRAM_RISCV_SIM_MACROS_H
//#define DRAM_RISCV_SIM_MACROS_H
//
//#endif //DRAM_RISCV_SIM_MACROS_H

#include <inttypes.h>
typedef uint32_t target_ulong;
typedef int32_t target_long;
#define GET_NUM_BITS(x) ceil(log(x)/log(2))