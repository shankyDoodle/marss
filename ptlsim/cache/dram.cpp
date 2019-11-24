//#define _GNU_SOURCE

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include <inttypes.h>
#include <errno.h>
#include <string.h>

#include "riscv_sim_macros.h"
#include "dram.h"

/**
 * Physical address to DRAM mapping: MSB-LSB
 * | row | rank | col_high | bank | col_low | dimm | bus_offset |
 */

#define PADDR_GET(paddr, left_shift_by, mask)                                  \
    (((paddr) >> (left_shift_by)) & (mask))

#define MASK(bits) ((1 << (bits)) - 1)

static int row_buffer_hit_latency[]
    = {ROW_BUFFER_READ_HIT_LATENCY, ROW_BUFFER_WRITE_HIT_LATENCY};

static int row_buffer_miss_latency[]
    = {ROW_BUFFER_READ_MISS_LATENCY, ROW_BUFFER_WRITE_MISS_LATENCY};

#define LAYER_COUNT 8
#define SHOW_DRAM_CONFIG 0 //{1/0 = Yes/No}

#define OUTPUT_FOR_SINGLE_LAYER 0
#define OUTPUT_FOR_MULTI_LAYER 0
#define OUTPUT_FOR_MULTI_LAYER_VAULT_ORG 1
#define COMPARE_ALL_OUTPUTS 0

Dram *dram_init(uint64_t size, int num_dimms, int num_banks, int mem_bus_width, int col_size) {
  Dram *d;
  int i, j, k, remaining_bits;

  d = (Dram*)calloc(1, sizeof(Dram));
  assert(d);
  d->dram_size = size;
  d->num_dimms = num_dimms;
  d->num_dimm_bits = GET_NUM_BITS(d->num_dimms);
  d->num_banks = num_banks;
  d->num_bank_bits = GET_NUM_BITS(d->num_banks);
  d->mem_bus_width = mem_bus_width;
  d->mem_bus_width_bytes = d->mem_bus_width / SIZE_OF_BYTE;

  d->col_size = col_size;
  d->num_paddr_bits = GET_NUM_BITS(d->dram_size);
//  remaining_bits = d->num_paddr_bits - (d->bus_offset_bits + d->num_dimm_bits + d->num_bank_bits + DRAM_NUM_RANK_BITS);
  d->num_row_bits = 4;//0.5 * remaining_bits;
  d->num_col_high_bits = 2;//0.7 * (remaining_bits - d->num_row_bits);
  d->num_col_low_bits = 2;//remaining_bits - (d->num_row_bits + d->num_col_high_bits);

  d->bus_offset_bits = d->num_paddr_bits - (d->num_dimm_bits +
                                            d->num_col_low_bits +
                                            d->num_bank_bits +
                                            d->num_col_high_bits +
                                            DRAM_NUM_RANK_BITS +
                                            d->num_row_bits);

  d->dimm = (DIMM*)calloc(d->num_dimms, sizeof(DIMM));
  assert(d->dimm);

  for (i = 0; i < d->num_dimms; ++i) {
    for (j = 0; j < DRAM_NUM_RANKS; ++j) {
      d->dimm[i].rank[j].chip.bank = (DramBank*)calloc(d->num_banks, sizeof(DramBank));
      assert(d->dimm[i].rank[j].chip.bank);
      for (k = 0; k < d->num_banks; ++k) {
        d->dimm[i].rank[j].chip.bank[k].last_accessed_row_id = -1;
      }
    }
  }

  return d;
}

/**
 * Maps the given paddr to DRAM row, checks for row-buffer hit or miss
 * and returns latency in CPU cycles accordingly.
 */
int dram_get_latency(Dram *d, target_ulong paddr, MemAccessType type) {
  int dimm_idx, bank_idx, rank_idx, row_idx;
  int latency = 0;

  /* Map the physical address into dimm, bank, rank and row */
  dimm_idx = PADDR_GET(paddr,
                       d->bus_offset_bits,
                       MASK(d->num_dimm_bits));

  bank_idx = PADDR_GET(paddr,
                       d->num_col_low_bits + d->num_dimm_bits + d->bus_offset_bits,
                       MASK(d->num_bank_bits));

  rank_idx = PADDR_GET(paddr,
                       d->num_col_high_bits + d->num_bank_bits + d->num_col_low_bits + d->num_dimm_bits + d->bus_offset_bits,
                       MASK(DRAM_NUM_RANK_BITS));

  row_idx = PADDR_GET(paddr,
                      DRAM_NUM_RANK_BITS + d->num_col_high_bits + d->num_bank_bits + d->num_col_low_bits + d->num_dimm_bits + d->bus_offset_bits,
                      MASK(d->num_row_bits));

  if (OUTPUT_FOR_MULTI_LAYER) {
    int hitInBuffer = 0;
    for (int i = 0; i < LAYER_COUNT; i++) {
      if (row_idx == vault2dBuffer[i]) {
        latency = 45 + (2 * (i+1));
        hitInBuffer = 1;
        break;
      }
    }
    if (hitInBuffer == 0) {
      latency = 90;
      int emptySPaceFound = 0;
      for (int i = 0; i < LAYER_COUNT; i++) {
        if (vault2dBuffer[i] == -1) {
          vault2dBuffer[i] = row_idx;
          emptySPaceFound = 1;
          lastStoredIndexValue2dBuffer = i;
          break;
        }
      }
      if (!emptySPaceFound) {
        if(lastStoredIndexValue2dBuffer >= LAYER_COUNT-1){
          lastStoredIndexValue2dBuffer = 0;
        }
        vault2dBuffer[lastStoredIndexValue2dBuffer] = row_idx;
      }
    }

    if(!COMPARE_ALL_OUTPUTS){
      return latency;
    }
  }

  if (OUTPUT_FOR_MULTI_LAYER_VAULT_ORG) {
    int hitInBuffer = 0;

    for (int i = 0; i < LAYER_COUNT; i++) {
      for (int j = 0; j < LAYER_COUNT; j++) {
        if (row_idx == vaultBuffer[i][j]) {
          latency = 45 + (2*(i+1));
          hitInBuffer = 1;
          break;
        }
      }
    }
    if (hitInBuffer == 0) {
      latency = 90;
      int emptySPaceFound = 0;
      for (int i = 0; i < LAYER_COUNT; i++) {
        for (int j = 0; j < LAYER_COUNT; j++) {
          if (vaultBuffer[i][j] == -1) {
            vaultBuffer[i][j] = row_idx;
            emptySPaceFound = 1;
            lastStoredIndexValueBufferI = i;
            lastStoredIndexValueBufferJ = j;
            break;
          }
        }
      }
      if (!emptySPaceFound) {
        if (lastStoredIndexValueBufferI < LAYER_COUNT-2 && lastStoredIndexValueBufferJ < LAYER_COUNT-2){
          lastStoredIndexValueBufferI++;
          lastStoredIndexValueBufferJ++;
        } else if(lastStoredIndexValueBufferJ >= LAYER_COUNT-1){
          lastStoredIndexValueBufferJ = 0;
          if(lastStoredIndexValueBufferI  == LAYER_COUNT-1){
            lastStoredIndexValueBufferI = 0;
          }else{
            lastStoredIndexValueBufferI++;
          }
        } else if(lastStoredIndexValueBufferI == LAYER_COUNT-1 && lastStoredIndexValueBufferJ == LAYER_COUNT-1){
          lastStoredIndexValueBufferI = 0;
          lastStoredIndexValueBufferJ = 0;
        }
        vaultBuffer[lastStoredIndexValueBufferI][lastStoredIndexValueBufferJ] = row_idx;

      }
    }
    if(!COMPARE_ALL_OUTPUTS){
      return latency;
    }
  }

  if(OUTPUT_FOR_SINGLE_LAYER){
    /* Get the bank in which the given paddr is mapped */
    DramBank *bank = &d->dimm[dimm_idx].rank[rank_idx].chip.bank[bank_idx];

    /* Check if DRAM page is already open in the row-buffer */
    if (bank->last_accessed_row_id == row_idx) {
      /* Row buffer hit (open-page) */
      latency = row_buffer_hit_latency[type];
    } else {
      /* Row buffer miss */
      latency = row_buffer_miss_latency[type];
      bank->last_accessed_row_id = row_idx;
    }
  }

  return latency;
}

void dram_print_config(Dram *d) {
  printf("%-15s = %llu\n", "dram_size", d->dram_size);
  printf("%-15s = %d\n", "num_paddr_bits", d->num_paddr_bits);
  printf("%-15s = %d\n", "num_dimms", d->num_dimms);
  printf("%-15s = %d\n", "num_dimm_bits", d->num_dimm_bits);
  printf("%-15s = %d\n", "num_banks", d->num_banks);
  printf("%-15s = %d\n", "num_bank_bits", d->num_bank_bits);
  printf("%-15s = %d\n", "num_row_bits", d->num_row_bits);
  printf("%-15s = %d\n", "num_col_high_bits", d->num_col_high_bits);
  printf("%-15s = %d\n", "num_col_low_bits", d->num_col_low_bits);
  printf("%-15s = %d bits\n", "mem_bus_width", d->mem_bus_width);
  printf("%-15s = %d\n", "bus_offset_bits", d->bus_offset_bits);
}

void dram_free(Dram **d) {
  int i, j;

  for (i = 0; i < (*d)->num_dimms; ++i) {
    for (j = 0; j < DRAM_NUM_RANKS; ++j) {
      free((*d)->dimm[i].rank[j].chip.bank);
      (*d)->dimm[i].rank[j].chip.bank = NULL;
    }
  }

  free((*d)->dimm);
  (*d)->dimm = NULL;
  free(*d);
  *d = NULL;
}


/** DRAM Initializer code start*/


int getCurrentLayer(target_ulong addr, int mem_bus_width) {
  int addressesInEachLayer = pow(2, mem_bus_width) / LAYER_COUNT;
  int layerNumber = floor(addr / addressesInEachLayer);
  return layerNumber;
}

int findMaxLatency(int layerBusy[]) {
  int max = 0;
  for (int i = 0; i < LAYER_COUNT; i++) {
    int j = layerBusy[i];
    if (max < layerBusy[i]) {
      max = layerBusy[i];
    }
  }
  return max;
}

int getVaultOrgLayer(target_ulong addr, int mem_bus_width) {
  int addressesInEachRegion = pow(2, mem_bus_width) / (LAYER_COUNT * LAYER_COUNT);
  int regionNumber = floor(addr / addressesInEachRegion);
  int layerNumber = ((regionNumber + 1) % LAYER_COUNT) - 1;
  layerNumber = layerNumber == -1 ? LAYER_COUNT-1 : layerNumber;
  return layerNumber;
}


Dram *dramInitializer() {
  uint64_t size = 4294967296;
  int num_dimms = 2;
  int num_banks = 8;
  int mem_bus_width = 32;
  int col_size = 4;
  return dram_init(size, num_dimms, num_banks, mem_bus_width, col_size);
}

int getCustomDRAMLatency(Dram *d, target_ulong paddr, MemAccessType type) {

  int mem_bus_width = 32;

  if (SHOW_DRAM_CONFIG) {
    printf("\n\n");
    printf("DRAM Config\n");
    dram_print_config(d);
    printf("\n\n");
  }

//  FILE *fp;
//  char *line = NULL;
//  size_t len = 0;
//  ssize_t read;
//  static const char filename[] = "paddr.txt";
//
//  fp = fopen(filename, "r");
//  if (fp == NULL)
//    exit(EXIT_FAILURE);


  int multiLayerBusy[LAYER_COUNT];
  if (OUTPUT_FOR_MULTI_LAYER || COMPARE_ALL_OUTPUTS) {
    for (int i = 0; i < LAYER_COUNT; i++) {
      multiLayerBusy[i] = 0;
    }
  }

  int vaultRegionBusy[LAYER_COUNT][LAYER_COUNT];
  int vault2dRegionBusy[LAYER_COUNT];
  if (OUTPUT_FOR_MULTI_LAYER_VAULT_ORG || COMPARE_ALL_OUTPUTS) {
    for (int i = 0; i < LAYER_COUNT; i++) {
      vault2dRegionBusy[i] = 0;

      //TODO: confirm behaviour
      for (int j = 0; j < LAYER_COUNT; j++) {
        vaultRegionBusy[i][j] = 0;
      }
    }
  }


  int totLatForSingleLayer = 0;
  int totLatForMultiLayer = 0;
  char *endptr;
//  while ((read = getline(&line, &len, fp)) != -1) {
//    target_ulong addr = strtoimax(line, &endptr, 2);
    int currentLatency = dram_get_latency(d, paddr, type);

    if (OUTPUT_FOR_SINGLE_LAYER || COMPARE_ALL_OUTPUTS) {
      totLatForSingleLayer += currentLatency;
    }

    if (OUTPUT_FOR_MULTI_LAYER || COMPARE_ALL_OUTPUTS) {
      int currentLayer = getCurrentLayer(paddr, mem_bus_width);
      multiLayerBusy[currentLayer] += currentLatency;
    }

    if (OUTPUT_FOR_MULTI_LAYER_VAULT_ORG || COMPARE_ALL_OUTPUTS) {
      int currentLayer = getVaultOrgLayer(paddr, mem_bus_width);
      vault2dRegionBusy[currentLayer] += currentLatency;
    }


//  }

//  fclose(fp);

  int retLat = 0;
  if (OUTPUT_FOR_SINGLE_LAYER || COMPARE_ALL_OUTPUTS) {
    //printf("Total Latency for Single-Layer Organisation is: %d\n", totLatForSingleLayer);
    retLat = totLatForSingleLayer;
  }
  if (OUTPUT_FOR_MULTI_LAYER || COMPARE_ALL_OUTPUTS) {
    totLatForMultiLayer = findMaxLatency(multiLayerBusy);
    //printf("Total Latency for Multi-Layer Organisation is: %d\n", totLatForMultiLayer);
    retLat = totLatForMultiLayer;
  }
  if (OUTPUT_FOR_MULTI_LAYER_VAULT_ORG || COMPARE_ALL_OUTPUTS) {
    totLatForMultiLayer = findMaxLatency(vault2dRegionBusy);
    //printf("Total Latency for Multi-Layer Vault Organisation is: %d\n", totLatForMultiLayer);
    retLat = totLatForMultiLayer;
  }


  return retLat;
}
