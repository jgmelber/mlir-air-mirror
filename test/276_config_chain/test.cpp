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

//#define BRAM_ADDR 0x4000+AIR_VCK190_SHMEM_BASE
#define BD_ADDR 0x3000+AIR_VCK190_SHMEM_BASE

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

  //volatile uint32_t *bram_ptr = (volatile uint32_t *)mmap(NULL, 0x8000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, BRAM_ADDR);
  XAieLib_MemInst *mem = XAieLib_MemAllocate(65536, XAIELIB_MEM_ATTR_CACHE);
  volatile uint32_t *bram_ptr = (volatile uint32_t *)XAieLib_MemGetVaddr(mem);
  uint32_t *paddr = (uint32_t *)XAieLib_MemGetPaddr(mem);
  volatile uint32_t *bd_ptr = (volatile uint32_t *)mmap(NULL, 0x8000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, BD_ADDR);

  uint64_t BRAM_ADDR = (uint64_t)paddr;

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

  for (int row = 0; row < 1; row++) {
    printf("\nWriting AIE shim %d,%d...\n",col,row);
    uint64_t core = getTileAddr(col,row);

    std::ifstream myfile;
    myfile.open ("core_"+std::to_string(col)+"_"+std::to_string(row)+"_fat_binary.txt");

    XAieLib_MemSyncForCPU(mem);

    int p = 0;
    int bd = 0;

    std::string line_desc;
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 23; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 23; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 92; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int b = 0; b < 16*5; b++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    uint32_t val;
    myfile >> std::hex >> val;
    bram_ptr[p++] = val;
    //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    myfile >> std::hex >> val;
    bram_ptr[p++] = val;
    //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 8; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    myfile.close();
  
    XAieLib_MemSyncForDev(mem);

    //uint64_t wr_idx = queue_add_write_index(q, 1);
    //uint64_t packet_id = wr_idx % q->size;
    //dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F000), uint64_t(BRAM_ADDR), 92);
    uint64_t next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    uint64_t sa = uint64_t(BRAM_ADDR);
    uint64_t da = uint64_t(core+0x0003F000);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 92;                    // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 92 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F100), uint64_t(BRAM_ADDR+92), 92);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+92);
    da = uint64_t(core+0x0003F100);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 92;                    // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 92 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F200), uint64_t(BRAM_ADDR+184), 368);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+184);
    da = uint64_t(core+0x0003F200);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 368;                   // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 368 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0001D000), uint64_t(BRAM_ADDR+552), 320);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+552);
    da = uint64_t(core+0x0001D000);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 320;                   // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 320 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt,  uint64_t(core+0x0001F000), uint64_t(BRAM_ADDR+872), 8);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+872);
    da = uint64_t(core+0x0001F000);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 8;                     // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 8 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0001D140), uint64_t(BRAM_ADDR+880), 32);
    sa = uint64_t(BRAM_ADDR+880);
    da = uint64_t(core+0x0001D140);
    bd_ptr[16*bd + 0] = 0; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = 0; // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 32;                    // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 32 << std::endl;

    uint64_t wr_idx = queue_add_write_index(q, 1);
    uint64_t packet_id = wr_idx % q->size;
    dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, next_ptr, uint64_t(BD_ADDR), 0xffffffff);
    pkt->type = 0x31;
    air_queue_dispatch_and_wait(q, wr_idx, pkt);

  }
  for (int row = 1; row < 3; row++) {
    printf("\nWriting AIE core %d,%d...\n",col,row);
    uint64_t core = getTileAddr(col,row);

    // FIXME
    for (int l=0; l<16; l++)
      XAieTile_LockRelease(&(xaie->TileInst[col][row]), l, 0x0, 0);

    std::ifstream myfile;
    myfile.open ("core_"+std::to_string(col)+"_"+std::to_string(row)+"_fat_binary.txt");

    XAieLib_MemSyncForCPU(mem);

    int p = 0;
    int bd = 0;

    XAieTile_CoreControl(&(xaie->TileInst[col][row]), XAIE_DISABLE, XAIE_ENABLE);

    std::string line_desc;
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 0x1000; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 25; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 27; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 100; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int b = 0; b < 16*8; b++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 8; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }
    std::getline(myfile, line_desc);
    std::getline(myfile, line_desc);
    std::cout << line_desc << " 0x" << std::hex << BRAM_ADDR+ (4*p) << std::endl;
    for (int i = 0; i < 0x1000; i++) {
      uint32_t val;
      myfile >> std::hex >> val;
      bram_ptr[p++] = val;
      //std::cout << std::hex << std::setw(8) << std::setfill('0') << val << std::endl;
    }

    XAieLib_MemSyncForDev(mem);

    //uint64_t wr_idx = queue_add_write_index(q, 1);
    //uint64_t packet_id = wr_idx % q->size;
    //dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x20000), uint64_t(BRAM_ADDR), 0x4000);
    uint64_t next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    uint64_t sa = uint64_t(BRAM_ADDR);
    uint64_t da = uint64_t(core+0x00020000);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 0x4000;                // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 0x4000 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F000), uint64_t(BRAM_ADDR), 100);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+16384);
    da = uint64_t(core+0x0003F000);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 100;                   // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 100 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F100), uint64_t(BRAM_ADDR+100), 108);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+16484);
    da = uint64_t(core+0x0003F100);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 108;                   // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 108 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0003F200), uint64_t(BRAM_ADDR+208), 400);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+16592);
    da = uint64_t(core+0x0003F200);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 400;                   // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 400 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0001D000), uint64_t(BRAM_ADDR+608), 512);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+16992);
    da = uint64_t(core+0x0001D000);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 512;                   // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 512 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0001DE00), uint64_t(BRAM_ADDR+1120), 32);
    next_ptr = uint64_t(BD_ADDR) + 64*(bd+1);
    sa = uint64_t(BRAM_ADDR+17504);
    da = uint64_t(core+0x0001DE00);
    bd_ptr[16*bd + 0] = next_ptr & 0xffffffff; // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = (next_ptr >> 32);      // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 32;                    // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << next_ptr << " SA: " << sa << " DA: " << da << " BTT: " << 32 << std::endl;
    bd++;

    //wr_idx = queue_add_write_index(q, 1);
    //packet_id = wr_idx % q->size;
    //pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    //air_packet_cdma_memcpy(pkt, uint64_t(core+0x0), uint64_t(BRAM_ADDR), 0x4000);
    sa = uint64_t(BRAM_ADDR+17536);
    da = uint64_t(core+0x00000000);
    bd_ptr[16*bd + 0] = 0;                     // NXTDESC_PNTR
    bd_ptr[16*bd + 1] = 0;                     // NXTDESC_PNTR_MSB
    bd_ptr[16*bd + 2] = sa & 0xffffffff;       // SA
    bd_ptr[16*bd + 3] = (sa >> 32);            // SA_MSB
    bd_ptr[16*bd + 4] = da & 0xffffffff;       // DA
    bd_ptr[16*bd + 5] = (da >> 32);            // DA_MSB
    bd_ptr[16*bd + 6] = 0x4000;                // CONTROL (BTT)
    bd_ptr[16*bd + 7] = 0;                     // STATUS
      std::cout << "Next: " << 0 << " SA: " << sa << " DA: " << da << " BTT: " << 0x4000 << std::endl;

    uint64_t wr_idx = queue_add_write_index(q, 1);
    uint64_t packet_id = wr_idx % q->size;
    dispatch_packet_t* pkt = (dispatch_packet_t*)(q->base_address_vaddr) + packet_id;
    air_packet_cdma_memcpy(pkt, next_ptr, uint64_t(BD_ADDR), 0xffffffff);
    pkt->type = 0x31;
    air_queue_dispatch_and_wait(q, wr_idx, pkt);

    XAieTile_CoreControl(&(xaie->TileInst[col][row]), XAIE_ENABLE, XAIE_DISABLE);

    myfile.close();
  }

  XAieLib_MemFree(mem);

  std::cout << std::endl << "PASS!" << std::endl;
  return 0;
}
