// (c) Copyright 2020 Xilinx Inc. All Rights Reserved.

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <xaiengine.h>

#include "air_host.h"
#include "acdc_queue.h"
#include "hsa_defs.h"
#include "aie_inc.cpp"

#define BAR_PF0_DEV_FILE_DDR    "/sys/bus/pci/devices/0000:21:00.0/resource0"
#define BAR_PF0_DEV_FILE_AIE    "/sys/bus/pci/devices/0000:21:00.0/resource2"
#define BAR_PF0_DEV_FILE_BRAM   "/sys/bus/pci/devices/0000:21:00.0/resource4" 

int
main(int argc, char *argv[])
{
  uint64_t row = 0;
  uint64_t col = 7;

  // Opening BAR2
  int fda;
  if((fda = open(BAR_PF0_DEV_FILE_AIE, O_RDWR | O_SYNC)) == -1) {
      printf("[ERROR] Failed to open device file\n");
      return 1;
  }
  printf("device file opened\n");

  // Map the memory region into userspace
  volatile void *map_aie_base = mmap(NULL,    // virtual address
                      0x20000000,             // length
                      PROT_READ | PROT_WRITE, // prot
                      MAP_SHARED,             // flags
                      fda,                    // device fd
                      0);                     // offset
  if (!map_aie_base) return 1;
  printf("AIE registers mapped into userspace at %p\n",map_aie_base);

  aie_libxaie_ctx_t *xaie = mlir_aie_init_libxaie(map_aie_base);
  mlir_aie_init_device(xaie);

  mlir_aie_configure_cores(xaie);
  mlir_aie_configure_switchboxes(xaie);
  mlir_aie_initialize_locks(xaie);
  mlir_aie_configure_dmas(xaie);
  mlir_aie_start_cores(xaie);

  volatile uint32_t *bram_ptr;

  #define BRAM_ADDR 0x800000
  #define DMA_COUNT 16

  int fd = open(BAR_PF0_DEV_FILE_DDR, O_RDWR | O_SYNC);
  if (fd != -1) {
    bram_ptr = (volatile uint32_t *)mmap(NULL, 0x2000000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i=0; i<DMA_COUNT; i++) {
      bram_ptr[i] = i+1;
      bram_ptr[DMA_COUNT+i] = 0xdeface;
      //printf("bbuf %p %u\n", &bram_ptr[i], bram_ptr[i]);
    }
  }

  for (int i=0; i<8; i++) {
    mlir_aie_write_buffer_ping_in(xaie, i, 0xabbaba00+i);
    mlir_aie_write_buffer_pong_in(xaie, i, 0xdeeded00+i);
    mlir_aie_write_buffer_ping_out(xaie, i, 0x12345670+i);
    mlir_aie_write_buffer_pong_out(xaie, i, 0x76543210+i);
  }

  // create the queue
  queue_t *q = nullptr;
  auto ret = air5000_queue_create(MB_QUEUE_SIZE, HSA_QUEUE_TYPE_SINGLE, &q, 
                                  0, const_cast<char *>(BAR_PF0_DEV_FILE_BRAM));
  assert(ret == 0 && "failed to create queue!");


  mlir_aie_print_tile_status(xaie,col,2);
  mlir_aie_print_dma_status(xaie,col,2);

  //
  // Set up a 1x3 herd starting 7,0
  //
  uint64_t wr_idx = queue_add_write_index(q, 1);
  uint64_t packet_id = wr_idx % q->size;
  //dispatch_packet_t *herd_pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //air_packet_herd_init(herd_pkt, 0, col, 1, row, 3);
  //air_queue_dispatch_and_wait(q, wr_idx, herd_pkt);

  //wr_idx = queue_add_write_index(q, 1);
  //packet_id = wr_idx % q->size;
  //dispatch_packet_t *shim_pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //air_packet_device_init(shim_pkt,XAIE_NUM_COLS);
  //air_queue_dispatch_and_wait(q, wr_idx, shim_pkt);

  //
  // send the data
  //

  //wr_idx = queue_add_write_index(q, 1);
  //packet_id = wr_idx % q->size;
  dispatch_packet_t *pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt, 0, col, 1, 0, 4, 2, BRAM_ADDR, DMA_COUNT*sizeof(float), 1, 0, 1, 0, 1, 0);

  //
  // read the data
  //

  wr_idx = queue_add_write_index(q, 1);
  packet_id = wr_idx % q->size;
  dispatch_packet_t *pkt2 = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt2, 0, col, 0, 0, 4, 2, BRAM_ADDR+(DMA_COUNT*sizeof(float)), DMA_COUNT*sizeof(float), 1, 0, 1, 0, 1, 0);
  air_queue_dispatch_and_wait(q, wr_idx, pkt2);

  mlir_aie_print_tile_status(xaie,col,2);
  mlir_aie_print_dma_status(xaie,col,2);

  int errors = 0;

  for (int i=0; i<8; i++) {
    uint32_t d0 = mlir_aie_read_buffer_ping_in(xaie, i);
    uint32_t d1 = mlir_aie_read_buffer_pong_in(xaie, i);
    uint32_t d2 = mlir_aie_read_buffer_ping_out(xaie, i);
    uint32_t d3 = mlir_aie_read_buffer_pong_out(xaie, i);
    if (d0+1 != d2) {
      printf("mismatch ping %x != %x\n", d0, d2);
      errors++;
    }
    if (d1+1 != d3) {
      printf("mismatch pong %x != %x\n", d1, d3);
      errors++;
    }
  }

  for (int i=0; i<DMA_COUNT; i++) {
    uint32_t d = bram_ptr[DMA_COUNT+i];
    if (d != (i+2)) {
      errors++;
      printf("mismatch %x != 2 + %x\n", d, i);
    }
  }
  if (!errors) {
    printf("PASS!\n");
    return 0;
  }
  else {
    printf("fail %d/%d.\n", errors, DMA_COUNT);
    return -1;
  }

}
