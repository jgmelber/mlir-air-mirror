// (c) Copyright 2021 Xilinx Inc. All Rights Reserved.

#include <assert.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <sys/mman.h>
#include <string>
#include <vector>
#include <xaiengine.h>

#include "acdc_queue.h"
#include "air_host.h"
#include "hsa_defs.h"

#define BRAM_ADDR 0x4000+AIR_VCK190_SHMEM_BASE

#define XAIE_ADDR_ARRAY_OFF     0x800

u64 getTileAddr(u16 ColIdx, u16 RowIdx) 
{
  u64 TileAddr = 0;
  u64 ArrOffset = XAIE_ADDR_ARRAY_OFF;

  #ifdef XAIE_BASE_ARRAY_ADDR_OFFSET
    ArrOffset = XAIE_BASE_ARRAY_ADDR_OFFSET;
  #endif

  /*
    * Tile address format:
    * --------------------------------------------
    * |                7 bits  5 bits   18 bits  |
    * --------------------------------------------
    * | Array offset | Column | Row | Tile addr  |
    * --------------------------------------------
    */
  TileAddr = (u64)((ArrOffset <<
      XAIEGBL_TILE_ADDR_ARR_SHIFT) |
    (ColIdx << XAIEGBL_TILE_ADDR_COL_SHIFT) |
    (RowIdx << XAIEGBL_TILE_ADDR_ROW_SHIFT));

  return TileAddr;
}

int main(int argc, char *argv[]) {
  aie_libxaie_ctx_t *xaie = mlir_aie_init_libxaie();
  mlir_aie_init_device(xaie);

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd == -1) {
    printf("failed to open /dev/mem\n");
    return -1;
  }

  volatile uint32_t *bram_ptr = (volatile uint32_t *)mmap(NULL, 0x8000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, BRAM_ADDR);
  //XAieLib_MemInst *mem = XAieLib_MemAllocate(65536, 0);
  //volatile uint32_t *bram_ptr = (volatile uint32_t *)XAieLib_MemGetVaddr(mem);
  //uint32_t *paddr = (uint32_t *)XAieLib_MemGetPaddr(mem);
  for (int i = 0; i < 0x1000; i++) {
    bram_ptr[i] = 0xabab;
  }
  //XAieLib_MemSyncForDev(mem);

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
    ret = air_queue_create(MB_QUEUE_SIZE, HSA_QUEUE_TYPE_SINGLE, &q,
                           agent.handle);
    assert(ret == 0 && "failed to create queue!");
    queues.push_back(q);
  }

  auto q = queues[0];
  int col = 7;

  //XAieTile_ShimColumnReset(&(xaie->TileInst[7][0]), XAIE_RESETENABLE);
  //XAieTile_ShimColumnReset(&(xaie->TileInst[7][0]), XAIE_RESETDISABLE);
  //XAieTile_PlShimResetEnable(&(xaie->TileInst[7][0]), XAIE_RESETENABLE);
  //XAieTile_PlShimResetEnable(&(xaie->TileInst[7][0]), XAIE_RESETDISABLE);

  for (int row = 0; row < 1; row++) {
    printf("\nReading AIE shim %d,%d...\n",col,row);
    uint64_t core = getTileAddr(col,row);

    std::ofstream myfile;
    myfile.open ("core_"+std::to_string(col)+"_"+std::to_string(row)+"_fat_binary.txt");

    uint64_t wr_idx = queue_add_write_index(q, 1);
    uint64_t packet_id = wr_idx % q->size;
    dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR), uint64_t(core+0x0003F000), 92);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+92), uint64_t(core+0x0003F100), 92);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+184), uint64_t(core+0x0003F200), 368);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+552), uint64_t(core+0x0001D000), 320);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+872), uint64_t(core+0x0001F000), 8);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+880), uint64_t(core+0x0001D140), 32);

    air_queue_dispatch_and_wait(q, wr_idx, pkt);

    //XAieLib_MemSyncForDev(mem);

    myfile << "SS Mstr:" << std::endl;
    for (int i = 0; i < 23; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i] << std::endl;
      //printf("BRAM %2d has %08X %08X\n",i,bram_ptr[i],0x3F000+i*4);
    }
    myfile << "SS Slve:" << std::endl;
    for (int i = 0; i < 23; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i+23] << std::endl;
      //printf("BRAM %2d has %08X %08X\n",i+23,bram_ptr[i+23],0x3F100+i*4);
    }
    myfile << "SS Pckt:" << std::endl;
    for (int i = 0; i < 92; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i+23+23] << std::endl;
      //printf("BRAM %2d has %08X %08X\n",i+23+23,bram_ptr[i+23+23],0x3F200+i*4);
    }
    myfile << "DMA BDs:" << std::endl;
    for (int b = 0; b < 16*5; b++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[b+23+23+92] << std::endl;
      //printf("BD %d:\n",b);
      //for (int i = 0; i < 5; i++) {
      //  printf("BRAM %2d has %08X %08X\n",b*5*4+i+23+23+92,bram_ptr[b*5+i+23+23+92],0x1D000+b*5*4+i*4);
      //}
    }
    myfile << "Shm Mux:" << std::endl;
    myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[23+23+92+16*5] << std::endl;
    myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[23+23+92+16*5+1] << std::endl;
    myfile << "DMA Ctl:" << std::endl;
    for (int i = 0; i < 8; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[23+23+92+16*5+2+i] << std::endl;
    }
  }
  //for (int row = 1; row < 3; row++) {
  //  printf("\nWriting AIE core %d,%d...\n",col,row);
  //  XAieTile_CoreControl(&(xaie->TileInst[col][row]), XAIE_DISABLE, XAIE_DISABLE);
  //  uint64_t core = getTileAddr(col,row);
  //  uint64_t wr_idx = queue_add_write_index(q, 1);
  //  uint64_t packet_id = wr_idx % q->size;
  //  dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //  air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F000), uint64_t(BRAM_ADDR), 100);

  //  wr_idx = queue_add_write_index(q, 1);
  //  packet_id = wr_idx % q->size;
  //  pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //  air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F100), uint64_t(BRAM_ADDR+100), 108);

  //  wr_idx = queue_add_write_index(q, 1);
  //  packet_id = wr_idx % q->size;
  //  pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //  air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F200), uint64_t(BRAM_ADDR+208), 400);

  //  wr_idx = queue_add_write_index(q, 1);
  //  packet_id = wr_idx % q->size;
  //  pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //  air_packet_cdma_memcpy(pkt, uint64_t(core+0x0001D000), uint64_t(BRAM_ADDR+608), 512);

  //  //uint64_t wr_idx = queue_add_write_index(q, 1);
  //  //uint64_t packet_id = wr_idx % q->size;
  //  //dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //  //air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR), uint64_t(core+0x0), 0x4000);

  //  //uint64_t wr_idx = queue_add_write_index(q, 1);
  //  //uint64_t packet_id = wr_idx % q->size;
  //  //dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
  //  //air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR), uint64_t(core+0x20000), 0x4000);

  //  air_queue_dispatch_and_wait(q, wr_idx, pkt);
  //}
  //for (int i = 0; i < 400; i++) {
  //  bram_ptr[i] = 0xABAB;
  //}
  for (int row = 1; row < 3; row++) {
    printf("\nReading AIE core %d,%d...\n",col,row);
    uint64_t core = getTileAddr(col,row);

    std::ofstream myfile;
    myfile.open ("core_"+std::to_string(col)+"_"+std::to_string(row)+"_fat_binary.txt");

    XAieTile_CoreControl(&(xaie->TileInst[col][row]), XAIE_DISABLE, XAIE_DISABLE);
    uint64_t wr_idx = queue_add_write_index(q, 1);
    uint64_t packet_id = wr_idx % q->size;
    dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR), uint64_t(core+0x20000), 0x4000);

    air_queue_dispatch_and_wait(q, wr_idx, pkt);

    myfile << "Pgm Mem:" << std::endl;
    for (int i = 0; i < 0x1000; i++) {
      uint32_t num = bram_ptr[i];
      myfile << std::hex << std::setw(8) << std::setfill('0') << num << std::endl;
      //uint32_t swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
      //                   ((num<<8)&0xff0000) | // move byte 1 to byte 2
      //                   ((num>>8)&0xff00) | // move byte 2 to byte 1
      //                   ((num<<24)&0xff000000); // byte 0 to byte 3
      //myfile << std::hex << std::setw(8) << std::setfill('0') << swapped << " ";
      //if ((i % 4) == 3) myfile << std::endl << "0x" << std::hex << std::setw(8) << std::setfill('0') << 4*(i+1) << " ";
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << swapped << " ";
      //if ((i % 4) == 3) std::cout << std::endl << "0x" << std::hex << std::setw(8) << std::setfill('0') << 4*(i+1) << " ";
    }

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR), uint64_t(core+0x0003F000), 100);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+100), uint64_t(core+0x0003F100), 108);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+208), uint64_t(core+0x0003F200), 400);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+608), uint64_t(core+0x0001D000), 512);

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR+1120), uint64_t(core+0x0001DE00), 32);

    air_queue_dispatch_and_wait(q, wr_idx, pkt);

    myfile << "SS Mstr:" << std::endl;
    for (int i = 0; i < 25; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i] << std::endl;
    }
    myfile << "SS Slve:" << std::endl;
    for (int i = 0; i < 27; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i+25] << std::endl;
    }
    myfile << "SS Pckt:" << std::endl;
    for (int i = 0; i < 100; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i+25+27] << std::endl;
    }
    myfile << "DMA BDs:" << std::endl;
    for (int b = 0; b < 16*8; b++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[b+25+27+100] << std::endl;
    }
    myfile << "DMA Ctl:" << std::endl;
    for (int i = 0; i < 8; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[25+27+100+16*8+i] << std::endl;
    }

    wr_idx = queue_add_write_index(q, 1);
    packet_id = wr_idx % q->size;
    pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, uint64_t(BRAM_ADDR), uint64_t(core+0x0), 0x4000);

    air_queue_dispatch_and_wait(q, wr_idx, pkt);

    myfile << "Dat Mem:" << std::endl;
    for (int i = 0; i < 0x1000; i++) {
      myfile << std::hex << std::setw(8) << std::setfill('0') << bram_ptr[i] << std::endl;
    }

    myfile.close();
  }
 
  //XAieLib_MemFree(mem);

  std::cout << std::endl << "PASS!" << std::endl;
  return 0;
}
