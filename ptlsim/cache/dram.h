/* This ifdef allows the header to be used from both C and C++. */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DRAM_H_
#define _DRAM_H_

#include <inttypes.h>

//#include "riscv_sim_macros.h"


//typedef uint32_t target_ulong;
//typedef int32_t target_long;
//#define GET_NUM_BITS(x) ceil(log((x)))


/* DRAM organization */
#define DRAM_NUM_DIMMS 1
#define DRAM_NUM_RANKS 2 /* Must always be 2, each side of DIMM represents one rank */
#define DRAM_NUM_RANK_BITS 1
#define DRAM_NUM_BANKS 8
#define DRAM_MEM_BUS_WIDTH 64 /* In bits */
#define DRAM_BANK_COL_SIZE 8 /* In bits */
#define SIZE_OF_BYTE 8 /* In bits */
#define LAYER_COUNT 8

/* CPU cycles required to drive address and data on memory bus(round-trip time) */
#define MEM_BUS_ACCESS_RTT_LATENCY 45

/* CPU cycles required to read a DRAM row into the row buffer */
#define ROW_READ_LATENCY 90

/* CPU cycles required to write contents of row buffer into the respective DRAM row */
#define ROW_WRITE_LATENCY 90

/* CPU cycles required to read data from row buffer */
#define ROW_BUFFER_READ_LATENCY 45

/* CPU cycles required to send the data to CPU on a row buffer hit */
#define ROW_BUFFER_READ_HIT_LATENCY (MEM_BUS_ACCESS_RTT_LATENCY + ROW_BUFFER_READ_LATENCY)

/* CPU cycles required to send the data to CPU on a row buffer miss */
#define ROW_BUFFER_READ_MISS_LATENCY (MEM_BUS_ACCESS_RTT_LATENCY + ROW_READ_LATENCY)

/* CPU cycles required to write data on a row buffer hit */
#define ROW_BUFFER_WRITE_HIT_LATENCY (MEM_BUS_ACCESS_RTT_LATENCY + ROW_BUFFER_READ_LATENCY + ROW_WRITE_LATENCY)

/* CPU cycles required to write data on a row buffer miss */
#define ROW_BUFFER_WRITE_MISS_LATENCY (MEM_BUS_ACCESS_RTT_LATENCY + ROW_READ_LATENCY + ROW_WRITE_LATENCY)


#define PADDR_GET(paddr, left_shift_by, mask)  (((paddr) >> (left_shift_by)) & (mask))

#define MASK(bits) ((1 << (bits)) - 1)

/* Memory operation type */
typedef enum MemAccessType {
    CustomRead = 0x0,
    CustomWrite = 0x1,
} MemAccessType;

/* Dram bank is logically a 2-D array of rows and columns.
 * For simplicity, keep track of only the last row accessed (row buffer). */
typedef struct DramBank {
    int last_accessed_row_id;
} DramBank;



typedef struct DramChip {
    DramBank *bank;
} DramChip;

typedef struct Rank {
    /* For simplicity, maintain a single chip to represent multiple chips in a
     * rank, since a given physical address is always mapped to the same bank in
     * all the chips in a given rank */
    DramChip chip;
} Rank;

typedef struct DualInLineMemoryModule {
    Rank rank[DRAM_NUM_RANKS];
} DIMM;

typedef struct MeshTiles{
    int lastSavedX;
    int lastSavedY;
} MTPos;

typedef struct Dram {
    uint64_t dram_size;         /* DRAM size in bytes */
    int num_paddr_bits;         /* Number of bits in physical address */
    int num_dimms;              /* Number of DIMMs */
    int num_dimm_bits;          /* Number of bits to access required DIMM */
    int num_banks;              /* Number of banks */
    int num_bank_bits;          /* Number of bits to access required bank */
    int num_row_bits;           /* Number of bits to access row in a bank */
    int num_col_high_bits;      /* Number of high-order bits to access column in a bank */
    int num_col_low_bits;       /* Number of low-order bits to access column in a bank */
    int col_size;               /* Number of bits in a column */
    int mem_bus_width;          /* Size of memory bus in bits */
    int mem_bus_width_bytes;    /* Size of memory bus in bytes */
    int bus_offset_bits;        /* Number of bits to access a byte within memory bus */
    DIMM *dimm;
    MTPos *mtPos;
} Dram;

Dram *dram_init(uint64_t size, int num_dimms, int num_banks, int mem_bus_width,
                int col_size);

void dram_free(Dram **d);

void dram_print_config(Dram *d);

int dram_get_latency(Dram *d, target_ulong paddr, MemAccessType type);

Dram *dramInitializer();

int getCustomDRAMLatency(Dram *d, target_ulong paddr, MemAccessType type);

#endif


#ifdef __cplusplus
}
#endif
