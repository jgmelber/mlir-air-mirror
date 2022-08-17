// (c) Copyright 2021 Xilinx Inc. All Rights Reserved.

#include <cassert>
#include <cstdint>  // uint64_t
#include <cstdio>   // printf
#include <cstdlib>  // atoi
#include <ctime>    // clock_gettime
#include <fcntl.h>  // open
#include <fstream>  // ifstream
#include <iomanip>  // setw, dec, hex
#include <iostream> // cout
#include <string>
#include <sys/mman.h>
#include <sys/time.h>
#include <vector>

#include <xaiengine.h>

#include "acdc_queue.h"
#include "air_host.h"
#include "hsa_defs.h"

const int ping_in_ofst = 4096;
int32_t mlir_aie_read_buffer_ping_in(aie_libxaie_ctx_t *ctx, int col, int row,
                                     int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      ping_in_ofst + (index * 4));
  return value;
}
void mlir_aie_write_buffer_ping_in(aie_libxaie_ctx_t *ctx, int col, int row,
                                   int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              ping_in_ofst + (index * 4), int_value);
}
const int ping_out_ofst = 4128;
int32_t mlir_aie_read_buffer_ping_out(aie_libxaie_ctx_t *ctx, int col, int row,
                                      int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      ping_out_ofst + (index * 4));
  return value;
}
void mlir_aie_write_buffer_ping_out(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              ping_out_ofst + (index * 4), int_value);
}
const int pong_in_ofst = 4160;
int32_t mlir_aie_read_buffer_pong_in(aie_libxaie_ctx_t *ctx, int col, int row,
                                     int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      pong_in_ofst + (index * 4));
  return value;
}
void mlir_aie_write_buffer_pong_in(aie_libxaie_ctx_t *ctx, int col, int row,
                                   int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              pong_in_ofst + (index * 4), int_value);
}
const int pong_out_ofst = 4192;
int32_t mlir_aie_read_buffer_pong_out(aie_libxaie_ctx_t *ctx, int col, int row,
                                      int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      pong_out_ofst + (index * 4));
  return value;
}
void mlir_aie_write_buffer_pong_out(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              pong_out_ofst + (index * 4), int_value);
}
const int ping_a_offset = 4096;
int32_t mlir_aie_read_buffer_ping_a(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      ping_a_offset + (index * 4));
  return value;
}
void mlir_aie_write_buffer_ping_a(aie_libxaie_ctx_t *ctx, int col, int row,
                                  int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              ping_a_offset + (index * 4), int_value);
}
const int ping_b_offset = 8192;
int32_t mlir_aie_read_buffer_ping_b(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      ping_b_offset + (index * 4));
  return value;
}
void mlir_aie_write_buffer_ping_b(aie_libxaie_ctx_t *ctx, int col, int row,
                                  int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              ping_b_offset + (index * 4), int_value);
}
const int ping_c_offset = 12288;
int32_t mlir_aie_read_buffer_ping_c(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      ping_c_offset + (index * 4));
  return value;
}
void mlir_aie_write_buffer_ping_c(aie_libxaie_ctx_t *ctx, int col, int row,
                                  int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              ping_c_offset + (index * 4), int_value);
}
const int pong_a_offset = 16384;
int32_t mlir_aie_read_buffer_pong_a(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      pong_a_offset + (index * 4));
  return value;
}
void mlir_aie_write_buffer_pong_a(aie_libxaie_ctx_t *ctx, int col, int row,
                                  int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              pong_a_offset + (index * 4), int_value);
}
const int pong_b_offset = 20480;
int32_t mlir_aie_read_buffer_pong_b(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      pong_b_offset + (index * 4));
  return value;
}
void mlir_aie_write_buffer_pong_b(aie_libxaie_ctx_t *ctx, int col, int row,
                                  int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              pong_b_offset + (index * 4), int_value);
}
const int pong_c_offset = 24576;
int32_t mlir_aie_read_buffer_pong_c(aie_libxaie_ctx_t *ctx, int col, int row,
                                    int index) {
  int32_t value = XAieTile_DmReadWord(&(ctx->TileInst[col][row]),
                                      pong_c_offset + (index * 4));
  return value;
}
void mlir_aie_write_buffer_pong_c(aie_libxaie_ctx_t *ctx, int col, int row,
                                  int index, int32_t value) {
  int32_t int_value = value;
  return XAieTile_DmWriteWord(&(ctx->TileInst[col][row]),
                              pong_c_offset + (index * 4), int_value);
}

static bool addone_driver(aie_libxaie_ctx_t *xaie, queue_t *q, int col,
                          int row) {
  /////////////////////////////////////////////////////////////////////////////
  //////////////////////// Run Add One Application ////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  static constexpr auto DMA_COUNT = 16ul;

  XAieLib_MemInst *mem2 =
      XAieLib_MemAllocate(2 * DMA_COUNT * sizeof(uint32_t), 0);
  uint32_t *bb_ptr = (uint32_t *)XAieLib_MemGetVaddr(mem2);
  uint32_t *bb_paddr = (uint32_t *)XAieLib_MemGetPaddr(mem2);

  XAieLib_MemSyncForCPU(mem2);
  if (mem2) {
    for (auto i = 0u; i < DMA_COUNT; i++) {
      bb_ptr[i] = i + 1;
      bb_ptr[DMA_COUNT + i] = 0xdeface;
    }
  } else {
    printf("ERROR: could not allocate memory!\n");
    return true;
  }
  XAieLib_MemSyncForDev(mem2);

  for (auto i = 0; i < 8; i++) {
    mlir_aie_write_buffer_ping_in(xaie, col, row, i, 0xabbaba00 + i);
    mlir_aie_write_buffer_pong_in(xaie, col, row, i, 0xdeeded00 + i);
    mlir_aie_write_buffer_ping_out(xaie, col, row, i, 0x12345670 + i);
    mlir_aie_write_buffer_pong_out(xaie, col, row, i, 0x76543210 + i);
  }

  mlir_aie_print_tile_status(xaie, col, row);
  mlir_aie_print_dma_status(xaie, col, row);

  //
  // send the data
  //

  uint64_t wr_idx = queue_add_write_index(q, 1);
  uint64_t packet_id = wr_idx % q->size;
  auto *pkt1 =
      reinterpret_cast<dispatch_packet_t *>(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt1, 0, col, 1, 0, 4, 2, (uint64_t)bb_paddr,
                       DMA_COUNT * sizeof(float), 1, 0, 1, 0, 1, 0);

  //
  // read the data
  //

  wr_idx = queue_add_write_index(q, 1);
  packet_id = wr_idx % q->size;
  dispatch_packet_t *pkt2 =
      (dispatch_packet_t *)(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt2, 0, col, 0, 0, 4, 2,
                       (uint64_t)bb_paddr + (DMA_COUNT * sizeof(float)),
                       DMA_COUNT * sizeof(float), 1, 0, 1, 0, 1, 0);
  air_queue_dispatch_and_wait(q, wr_idx, pkt2);

  mlir_aie_print_tile_status(xaie, col, row);
  mlir_aie_print_dma_status(xaie, col, row);

  int errors = 0;

  for (int i = 0; i < 8; i++) {
    uint32_t d0 = mlir_aie_read_buffer_ping_in(xaie, col, row, i);
    uint32_t d1 = mlir_aie_read_buffer_pong_in(xaie, col, row, i);
    uint32_t d2 = mlir_aie_read_buffer_ping_out(xaie, col, row, i);
    uint32_t d3 = mlir_aie_read_buffer_pong_out(xaie, col, row, i);
    if (d0 + 1 != d2) {
      printf("mismatch ping %x != %x\n", d0, d2);
      errors++;
    }
    if (d1 + 1 != d3) {
      printf("mismatch pong %x != %x\n", d1, d3);
      errors++;
    }
  }

  XAieLib_MemSyncForCPU(mem2);
  for (auto i = 0u; i < DMA_COUNT; i++) {
    uint32_t d = bb_ptr[DMA_COUNT + i];
    if (d != (i + 2)) {
      errors++;
      printf("mismatch %x != 2 + %x\n", d, i);
    }
  }

  XAieLib_MemFree(mem2);

  if (!errors) {
    return 0;
  } else {
    return 1;
  }
}

static int matadd_driver(aie_libxaie_ctx_t *xaie, queue_t *q, int col,
                         int row) {
  // test configuration
  static constexpr auto IMAGE_WIDTH = 192;
  static constexpr auto IMAGE_HEIGHT = 192;
  static constexpr auto IMAGE_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT;

  static constexpr auto TILE_WIDTH = 32;
  static constexpr auto TILE_HEIGHT = 32;
  static constexpr auto TILE_SIZE = TILE_WIDTH * TILE_HEIGHT;

  static constexpr auto NUM_3D = IMAGE_WIDTH / TILE_WIDTH;
  static constexpr auto NUM_4D = IMAGE_HEIGHT / TILE_HEIGHT;

  XAieLib_MemInst *mem2 =
      XAieLib_MemAllocate(3 * IMAGE_SIZE * sizeof(uint32_t), 0);
  uint32_t *dram_ptr = (uint32_t *)XAieLib_MemGetVaddr(mem2);
  uint32_t *dram_paddr = (uint32_t *)XAieLib_MemGetPaddr(mem2);

  XAieLib_MemSyncForCPU(mem2);
  if (mem2) {
    for (int i = 0; i < IMAGE_SIZE; i++) {
      dram_ptr[i] = i + 1;
      dram_ptr[IMAGE_SIZE + i] = i + 2;
      dram_ptr[2 * IMAGE_SIZE + i] = 0xdeface;
    }
  } else {
    printf("ERROR: could not allocate memory!\n");
    return 1;
  }
  XAieLib_MemSyncForDev(mem2);

  // stamp over the aie tiles
  for (int i = 0; i < TILE_SIZE; i++) {
    mlir_aie_write_buffer_ping_a(xaie, col, row, i, 0xabba0000 + i);
    mlir_aie_write_buffer_pong_a(xaie, col, row, i, 0xdeeded00 + i);
    mlir_aie_write_buffer_ping_b(xaie, col, row, i, 0xcafe0000 + i);
    mlir_aie_write_buffer_pong_b(xaie, col, row, i, 0xfabcab00 + i);
    mlir_aie_write_buffer_ping_c(xaie, col, row, i, 0x12345670 + i);
    mlir_aie_write_buffer_pong_c(xaie, col, row, i, 0x76543210 + i);
  }

  mlir_aie_print_tile_status(xaie, col, row);
  mlir_aie_print_dma_status(xaie, col, row);

  //
  // packet to read the output matrix
  //

  uint64_t wr_idx = queue_add_write_index(q, 1);
  uint64_t packet_id = wr_idx % q->size;
  dispatch_packet_t *pkt_c =
      (dispatch_packet_t *)(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt_c, 0, col, 0, 0, 4, 2,
                       uint64_t(dram_paddr) + (2 * IMAGE_SIZE * sizeof(float)),
                       TILE_WIDTH * sizeof(float), TILE_HEIGHT,
                       IMAGE_WIDTH * sizeof(float), NUM_3D,
                       TILE_WIDTH * sizeof(float), NUM_4D,
                       IMAGE_WIDTH * TILE_HEIGHT * sizeof(float));

  //
  // packet to send the input matrices
  //

  wr_idx = queue_add_write_index(q, 1);
  packet_id = wr_idx % q->size;
  auto *pkt_a = (dispatch_packet_t *)(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt_a, 0, col, 1, 0, 4, 2, uint64_t(dram_paddr),
                       TILE_WIDTH * sizeof(float), TILE_HEIGHT,
                       IMAGE_WIDTH * sizeof(float), NUM_3D,
                       TILE_WIDTH * sizeof(float), NUM_4D,
                       IMAGE_WIDTH * TILE_HEIGHT * sizeof(float));

  wr_idx = queue_add_write_index(q, 1);
  packet_id = wr_idx % q->size;
  auto *pkt_b = (dispatch_packet_t *)(q->base_address_vaddr) + packet_id;
  air_packet_nd_memcpy(pkt_b, 0, col, 1, 1, 4, 2,
                       uint64_t(dram_paddr) + (IMAGE_SIZE * sizeof(float)),
                       TILE_WIDTH * sizeof(float), TILE_HEIGHT,
                       IMAGE_WIDTH * sizeof(float), NUM_3D,
                       TILE_WIDTH * sizeof(float), NUM_4D,
                       IMAGE_WIDTH * TILE_HEIGHT * sizeof(float));

  //
  // dispatch the packets to the MB
  //

  air_queue_dispatch_and_wait(q, wr_idx, pkt_c);

  mlir_aie_print_tile_status(xaie, col, row);
  mlir_aie_print_dma_status(xaie, col, row);

  int errors = 0;
  // check the aie tiles
  for (int i = 0; i < TILE_SIZE; i++) {
    uint32_t d0 = mlir_aie_read_buffer_ping_a(xaie, col, row, i);
    uint32_t d1 = mlir_aie_read_buffer_pong_a(xaie, col, row, i);
    uint32_t d4 = mlir_aie_read_buffer_ping_b(xaie, col, row, i);
    uint32_t d5 = mlir_aie_read_buffer_pong_b(xaie, col, row, i);
    uint32_t d2 = mlir_aie_read_buffer_ping_c(xaie, col, row, i);
    uint32_t d3 = mlir_aie_read_buffer_pong_c(xaie, col, row, i);
    if (d0 + d4 != d2) {
      printf("mismatch [%d] ping %x+%x != %x\n", i, d0, d4, d2);
      errors++;
    }
    if (d1 + d5 != d3) {
      printf("mismatch [%d] pong %x+%x != %x\n", i, d1, d5, d3);
      errors++;
    }
  }

  // check the output image
  for (auto i = 0u; i < IMAGE_SIZE; i++) {
    auto d = dram_ptr[2u * IMAGE_SIZE + i];
    if (d != i + 1 + i + 2) {
      errors++;
      printf("mismatch %x != %x\n", d, 2 * (i + 1));
    }
  }

  XAieLib_MemFree(mem2);

  if (!errors) {
    return 0;
  } else {
    return 1;
  }
}

int main(int argc, char **argv) {
  uint8_t col = 7;
  int row = 2;
  std::string airbin_name;

  if (argc > 3) {
    std::cout << "Usage: " << *argv << " [airbin file] [column number]"
              << std::endl;
    return 1;
  } else if (argc == 3) {
    if (auto column_input = atoi(argv[2]);
        column_input > 0 and column_input < UINT8_MAX) {
      col = column_input;
    } else {
      std::cout << "Error: " << argv[2] << " must be between 0 and "
                << UINT8_MAX << " inclusive" << std::endl;
      return 2;
    }
    airbin_name = argv[1];
  } else if (argc == 2) {
    airbin_name = argv[1];
  } else {
    airbin_name = "addone.airbin";
  }

  std::cout << "\nConfiguring herd in col " << col << "..." << std::endl;

  aie_libxaie_ctx_t *xaie = mlir_aie_init_libxaie();
  mlir_aie_init_device(xaie);

  XAieTile_ShimColumnReset(&(xaie->TileInst[7][0]), XAIE_RESETENABLE);
  XAieTile_ShimColumnReset(&(xaie->TileInst[7][0]), XAIE_RESETDISABLE);
  for (auto i = 1u; i <= 2u; ++i)
    XAieTile_CoreControl(&(xaie->TileInst[7][i]), XAIE_DISABLE, XAIE_ENABLE);

  std::vector<air_agent_t> agents;
  auto ret = air_get_agents(&agents);
  assert(ret == 0 && "failed to get agents!");

  if (agents.empty()) {
    std::cout << "fail." << std::endl;
    return -1;
  }

  std::vector<queue_t *> queues;
  for (auto agent : agents) {
    // create the queue
    queue_t *q = nullptr;
    auto ret = air_queue_create(MB_QUEUE_SIZE, HSA_QUEUE_TYPE_SINGLE, &q,
                                agent.handle);
    assert(ret == 0 && "failed to create queue!");
    queues.push_back(q);
  }

  auto *q = queues[0];

  if(air_load_airbin(q, airbin_name.cstr(), col) != 0){
	  std::cout << "Error loading airbin" << std::endl;
	  return 1;
  }

  for (auto i = 1u; i <= 2u; ++i) {
    XAieTile_CoreControl(&(xaie->TileInst[7][i]), XAIE_ENABLE, XAIE_DISABLE);
    for (int l = 0; l < 16; l++)
      XAieTile_LockRelease(&(xaie->TileInst[7][i]), l, 0x0, 0);
  }

  std::cout << "\nDone configuring!" << std::endl << std::endl;

  int errors = 0;

  if (airbin_name == "addone.airbin") {
    errors = addone_driver(xaie, q, col, row);
  } else if (airbin_name == "matadd.airbin") {
    errors = matadd_driver(xaie, q, col, row);
  } else
    errors = 1;

  XAieLib_MemFree(mem);

  infile.close();

  if (!errors) {
    std::cout << "PASS!" << std::endl;
    return 0;
  } else {
    std::cout << "fail." << std::endl;
    return -1;
  }
}
