// (c) Copyright 2021 Xilinx Inc. All Rights Reserved.

#include <assert.h>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <vector>

#include "../../airhost/include/acdc_queue.h"
#include "../../airhost/include/air_host.h"
#include "../../airhost/include/hsa_defs.h"

#define BAR_PF0_DEV_FILE_DDR    "/sys/bus/pci/devices/0000:21:00.0/resource2" 
// Change to resource4 for testing as that memory is just backed by DDR
#define BAR_PF0_DEV_FILE_BRAM   "/sys/bus/pci/devices/0000:21:00.0/resource4" 
// Change to resource4 for testing as that memory is just backed by BRAM

// Defined in acdc_queue.h
//typedef enum {
//  AIR_AGENT_INFO_NAME = 0,        // NUL-terminated char[8]
//  AIR_AGENT_INFO_VENDOR_NAME = 1, // NUL-terminated char[8]
//  AIR_AGENT_INFO_CONTROLLER_ID = 2,
//  AIR_AGENT_INFO_FIRMWARE_VER = 3,
//  AIR_AGENT_INFO_NUM_REGIONS = 4,
//  AIR_AGENT_INFO_HERD_SIZE = 5,
//  AIR_AGENT_INFO_HERD_ROWS = 6,
//  AIR_AGENT_INFO_HERD_COLS = 7,
//  AIR_AGENT_INFO_TILE_DATA_MEM_SIZE = 8,
//  AIR_AGENT_INFO_TILE_PROG_MEM_SIZE = 9,
//  AIR_AGENT_INFO_L2_MEM_SIZE = 10 // Per region
//} air_agent_info_t;

int main(int argc, char *argv[]) {
  std::vector<air_agent_t> agents;
  //auto ret = air_get_agents(&agents);
  //assert(ret == 0 && "failed to get agents!");
  air_agent_t a1;
  a1.handle = 0;
  agents.push_back(a1);

  if (agents.empty()) {
    std::cout << "fail." << std::endl;
    return -1;
  }

  char bar_dev_file_BRAM[100];
  strcpy(bar_dev_file_BRAM, BAR_PF0_DEV_FILE_BRAM);
  printf("Opening %s to access MMIO BRAM\n", bar_dev_file_BRAM);

  std::vector<queue_t *> queues;
  for (auto agent : agents) {
    // create the queue
    queue_t *q = nullptr;
    auto ret = air5000_queue_create(MB_QUEUE_SIZE, HSA_QUEUE_TYPE_SINGLE, &q,
                                    agent.handle, bar_dev_file_BRAM);
    printf("Created queue @ %lx\n",q);
    assert(ret == 0 && "failed to create queue!");
    queues.push_back(q);
  }

  // reserve a packet in the queue
  uint64_t wr_idx = queue_add_write_index(queues[0], 1);
  uint64_t packet_id = wr_idx % queues[0]->size;

  dispatch_packet_t *pkt = (dispatch_packet_t*)(queues[0]->base_address_vaddr) + packet_id;
  air_packet_hello(pkt, 0xacdc0000LL + packet_id);
  air_queue_dispatch_and_wait(queues[0], wr_idx, pkt);
     
  uint64_t data = -1;
  char vend[8];
  for (auto q : queues) {
    std::cout << std::endl << "Requesting attribute: AIR_AGENT_INFO_CONTROLLER_ID... ";
    air_get_agent_info(q, AIR_AGENT_INFO_CONTROLLER_ID, &data);
    std::cout << "Agent ID is: " << data << std::endl;

    std::cout << "Requesting attribute: AIR_AGENT_INFO_VENDOR_NAME... ";
    air_get_agent_info(q, AIR_AGENT_INFO_VENDOR_NAME, vend);
    std::cout << "Vendor is: " << vend << std::endl;

    std::cout << "Requesting attribute: AIR_AGENT_INFO_L2_MEM_SIZE... ";
    air_get_agent_info(q, AIR_AGENT_INFO_L2_MEM_SIZE, &data);
    std::cout << "L2 size is: " << std::dec << data << "B" << std::endl;
  }

  std::cout << std::endl << "PASS!" << std::endl;
  return 0;
}
