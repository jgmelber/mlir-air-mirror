#include "air_host.h"

#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string>

//#define XAIE_NUM_ROWS 8
//#define XAIE_NUM_COLS 50

// temporary solution to stash some state
extern "C" {

air_rt_herd_desc_t _air_host_active_herd = {nullptr, nullptr};
aie_libxaie_ctx_t *_air_host_active_libxaie1 = nullptr;
uint32_t *_air_host_bram_ptr = nullptr;
air_module_handle_t _air_host_active_module = (air_module_handle_t)nullptr;

}

volatile void *_mapped_aie_base = nullptr;

aie_libxaie_ctx_t *
air_init_libxaie1()
{
  if (_air_host_active_libxaie1)
    return _air_host_active_libxaie1;

  aie_libxaie_ctx_t *xaie =
    (aie_libxaie_ctx_t*)malloc(sizeof(aie_libxaie_ctx_t));
  if (!xaie)
    return 0;

  xaie->AieConfigPtr.AieGen = XAIE_DEV_GEN_AIE;
//#ifdef AIR_PCIE
  std::string aie_bar = air_get_aie_bar();

  int fda;
  if((fda = open(aie_bar.c_str(), O_RDWR | O_SYNC)) == -1) {
      printf("[ERROR] Failed to open device file\n");
      return nullptr;
  }

  // Map the memory region into userspace
  _mapped_aie_base = mmap(NULL,               // virtual address
                      0x20000000,             // length
                      PROT_READ | PROT_WRITE, // prot
                      MAP_SHARED,             // flags
                      fda,                    // device fd
                      0);                     // offset
  if (!_mapped_aie_base) return nullptr;
  xaie->AieConfigPtr.BaseAddr = (uint64_t)_mapped_aie_base;
//#else
//  xaie->AieConfigPtr.BaseAddr = XAIE_BASE_ADDR;
//#endif
  xaie->AieConfigPtr.ColShift = XAIE_COL_SHIFT;
  xaie->AieConfigPtr.RowShift = XAIE_ROW_SHIFT;
  xaie->AieConfigPtr.NumRows = XAIE_NUM_ROWS;
  xaie->AieConfigPtr.NumCols = XAIE_NUM_COLS;
  xaie->AieConfigPtr.ShimRowNum = XAIE_SHIM_ROW;
  xaie->AieConfigPtr.MemTileRowStart = XAIE_RES_TILE_ROW_START;
  xaie->AieConfigPtr.MemTileNumRows = XAIE_RES_TILE_NUM_ROWS;
  xaie->AieConfigPtr.AieTileRowStart = XAIE_AIE_TILE_ROW_START;
  xaie->AieConfigPtr.AieTileNumRows = XAIE_AIE_TILE_NUM_ROWS;
  xaie->AieConfigPtr.PartProp = {0};
  xaie->DevInst = {0};

  XAie_CfgInitialize(&(xaie->DevInst), &(xaie->AieConfigPtr));
  XAie_PmRequestTiles(&(xaie->DevInst), NULL, 0);
  XAie_Finish(&(xaie->DevInst));
  XAie_CfgInitialize(&(xaie->DevInst), &(xaie->AieConfigPtr));
  XAie_PmRequestTiles(&(xaie->DevInst), NULL, 0);

  //XAIEGBL_HWCFG_SET_CONFIG((&xaie->AieConfig),
  //                         XAIE_NUM_ROWS, XAIE_NUM_COLS, 0x800);
  //XAieGbl_HwInit(&xaie->AieConfig);
  //xaie->AieConfigPtr = XAieGbl_LookupConfig(XPAR_AIE_DEVICE_ID);
  //XAieGbl_CfgInitialize(&xaie->AieInst,
  //                      &xaie->TileInst[0][0], xaie->AieConfigPtr);

  _air_host_active_libxaie1 = xaie;
  return xaie;
}

void
air_deinit_libxaie1(aie_libxaie_ctx_t *xaie)
{
  if (xaie == _air_host_active_libxaie1) {
    XAie_Finish(&(xaie->DevInst));
    munmap(const_cast<void*>(_mapped_aie_base),0x20000000);
    _mapped_aie_base = nullptr;
    _air_host_active_libxaie1 = nullptr;
  }
  free(xaie);
}

air_module_handle_t
air_module_load_from_file(const char* filename, queue_t *q)
{

  if (_air_host_active_module)
    air_module_unload(_air_host_active_module);

  air_module_handle_t handle;
  void* _handle = dlopen(filename, RTLD_NOW);
  if (!_handle) {
    printf("%s\n",dlerror());
    return 0;
  }
  _air_host_active_module = (air_module_handle_t)_handle;

  auto module_desc = air_module_get_desc(_air_host_active_module);

  if (module_desc->length)
    _air_host_active_herd.herd_desc = module_desc->herd_descs[0];
  _air_host_active_herd.q = q;

  assert(_air_host_active_herd.herd_desc);

//#ifdef AIR_PCIE
  int fd = open(air_get_bram_bar().c_str(), O_RDWR | O_SYNC);
  assert(fd != -1 && "Failed to open bram fd");

  _air_host_bram_ptr = (uint32_t *)mmap(NULL, 0x8000, PROT_READ|PROT_WRITE,
                                        MAP_SHARED, fd,
                                        0x4000);
  assert(_air_host_bram_ptr && "Failed to map scratch bram location");
//#else
//  int fd = open("/dev/mem", O_RDWR | O_SYNC);
//  assert(fd != -1 && "Failed to open bram fd");
//
//  _air_host_bram_ptr = (uint32_t *)mmap(NULL, 0x8000, PROT_READ|PROT_WRITE,
//                                        MAP_SHARED, fd,
//                                        AIR_VCK190_SHMEM_BASE+0x4000);
//  assert(_air_host_bram_ptr && "Failed to map scratch bram location");
//#endif

  return (air_module_handle_t)_handle;
}

int32_t
air_module_unload(air_module_handle_t handle)
{
  if (!handle)
    return -1;

  if (auto module_desc = air_module_get_desc(handle)) {
    for (int i=0; i<module_desc->length; i++) {
      auto herd_desc = module_desc->herd_descs[i];
      if (herd_desc == _air_host_active_herd.herd_desc) {
        _air_host_active_herd.herd_desc = nullptr;
        _air_host_active_herd.q = nullptr;
      }
    }
  }
  if (_air_host_active_module == handle)
    _air_host_active_module = (air_module_handle_t)nullptr;

  return dlclose((void*)handle);
}

air_herd_desc_t *
air_herd_get_desc(air_module_handle_t handle, const char *herd_name)
{
  if (!handle) return nullptr;

  auto module_desc = air_module_get_desc(handle);
  if (!module_desc) return nullptr;

  for (int i=0; i<module_desc->length; i++) {
    auto herd_desc = module_desc->herd_descs[i];
    if (!strncmp(herd_name, herd_desc->name, herd_desc->name_length))
      return herd_desc;
  }
  return nullptr;
}

air_module_desc_t *
air_module_get_desc(air_module_handle_t handle)
{
  if (!handle) return nullptr;
  return (air_module_desc_t*)dlsym((void*)handle, "__air_module_descriptor");
}

uint64_t
air_herd_load(const char *name) {
  auto herd_desc = air_herd_get_desc(_air_host_active_module, name);
  if (!herd_desc) {
    printf("Failed to locate herd descriptor '%s'!\n",name);
    assert(0);
  }
  std::string herd_name(herd_desc->name, herd_desc->name_length);

  // bool configured = (_air_host_active_herd.herd_desc == herd_desc);
  // if (configured) return 0;
  
  _air_host_active_herd.herd_desc = herd_desc;

  std::string func_name = "__airrt_" +
                          herd_name +
                          "_aie_functions";
  air_rt_aie_functions_t *mlir = 
    (air_rt_aie_functions_t *)dlsym((void*)_air_host_active_module,
                                    func_name.c_str());

  if (mlir) {
    //printf("configuring herd: '%s'\n",herd_name.c_str());
    assert(mlir->configure_cores);
    assert(mlir->configure_switchboxes);
    assert(mlir->initialize_locks);
    assert(mlir->configure_dmas);
    assert(mlir->start_cores);
    mlir->configure_cores(_air_host_active_libxaie1);
    mlir->configure_switchboxes(_air_host_active_libxaie1);
    mlir->initialize_locks(_air_host_active_libxaie1);
    mlir->configure_dmas(_air_host_active_libxaie1);
    mlir->start_cores(_air_host_active_libxaie1);
  }
  else {
    printf("Failed to locate herd '%s' configuration functions!\n",herd_name.c_str());
    assert(0);
  }

  return 0;
}

std::string air_get_ddr_bar() {
  return "/sys/bus/pci/devices/0000:21:00.0/resource0";
}
std::string air_get_aie_bar() {
  return "/sys/bus/pci/devices/0000:21:00.0/resource2";
}
std::string air_get_bram_bar() {
  return "/sys/bus/pci/devices/0000:21:00.0/resource4";
}
