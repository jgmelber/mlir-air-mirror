
#include "include/airbin.h"
#include "include/acdc_queue.h"
#include <cassert>
#include <cstdint>
#include <fstream>

// Note that the file header we store ignores the magic bytes.
struct Air64_Fhdr {
  uint16_t f_type;
  uint16_t arch;
  uint16_t f_ver;
  uint16_t num_ch;
  uint32_t chcfg;
};

static Air64_Fhdr read_file_header(std::ifstream &infile) {
  Air64_Fhdr result;
  unsigned char longnum[8] = {0};
  infile.seekg(2 * sizeof(longnum));

  infile.read(reinterpret_cast<char *>(longnum), 2);
  result.f_type = std::stoi(reinterpret_cast<char *>(longnum), NULL, 16);

  infile.read(reinterpret_cast<char *>(longnum), 2);
  result.arch = std::stoi(reinterpret_cast<char *>(longnum), NULL, 16);

  infile.read(reinterpret_cast<char *>(longnum), 2);
  result.f_ver = std::stoi(reinterpret_cast<char *>(longnum), NULL, 16);

  infile.read(reinterpret_cast<char *>(longnum), 2);
  result.num_ch = std::stoi(reinterpret_cast<char *>(longnum), NULL, 16);

  infile.read(reinterpret_cast<char *>(longnum), 8);
  result.chcfg = std::stoi(reinterpret_cast<char *>(longnum), NULL, 16);

  return result;
}

struct Air64_Chdr {
  uint16_t ch_tile;
  uint8_t ch_name;
  uint32_t ch_type;
  uint64_t ch_addr;
  uint64_t ch_offset;
  uint64_t ch_size;
};

static Air64_Chdr read_section_header(std::ifstream &infile,
                                      uint8_t column_offset) {
  Air64_Chdr config_header;
  uint32_t location_and_name;

  auto start = infile.tellg();
  infile >> std::hex >> location_and_name;
  infile >> std::hex >> config_header.ch_type;
  infile >> std::hex >> config_header.ch_addr;
  infile >> std::hex >> config_header.ch_addr;
  infile >> std::hex >> config_header.ch_offset;
  infile >> std::hex >> config_header.ch_offset;
  infile >> std::hex >> config_header.ch_size;
  infile >> std::hex >> config_header.ch_size;

  // Each byte needs 2 hex digits to represent.
  // Each 4-byte word (except the last) also needs a newline.
  static constexpr auto section_header_size =
      sizeof(config_header) * 2 + sizeof(config_header) / 4 - 1;
  // TODO: Better error messages
  assert(infile.tellg() - start == section_header_size);

  config_header.ch_name = location_and_name & 0xFFu;
  config_header.ch_tile = (location_and_name >> 8u) & 0xFFFFu;
  config_header.ch_tile += static_cast<uint16_t>(column_offset) << 5u;

  assert(config_header.ch_tile != 0);
  assert(config_header.ch_size % 4 == 0);

  return config_header;
}

uint64_t airbin2mem(std::ifstream &infile, volatile uint32_t *tds_va,
                    uint32_t *tds_pa, volatile uint32_t *data_va,
                    uint32_t *data_pa, uint8_t col) {
  uint64_t last_td = 0;

  uint32_t next_chcfg_idx = 0;
  Air64_Fhdr file_header = read_file_header(infile);

  int p = 0;
  int bd = 0;
  uint64_t next_ptr = uint64_t(tds_pa) + 64 * (bd + 1);
  uint64_t sa = uint64_t(data_pa);

  while (next_chcfg_idx < file_header.num_ch) {
    infile.seekg(file_header.chcfg + 1 + next_chcfg_idx * 9 * 8);
    Air64_Chdr config_header = read_section_header(infile, col);
    infile.seekg(config_header.ch_offset);
    if (config_header.ch_type) {
      if (config_header.ch_name != 11) {
        std::string line_desc;
        std::getline(infile, line_desc);
      }
      for (auto i = 0ul; i < config_header.ch_size / sizeof(uint32_t); i++) {
        uint32_t val;
        infile >> std::hex >> val;
        data_va[p++] = val;
      }
      if (config_header.ch_name != 11) {
        std::string line_desc;
        std::getline(infile, line_desc);
      }
      auto core =
          array_offset | (static_cast<uint64_t>(config_header.ch_tile) << 18u);
      uint64_t da = uint64_t(core + config_header.ch_addr);
      assert((da & core) == core);
      tds_va[16 * bd + 0] = next_ptr & 0xffffffffu; // NXTDESC_PNTR
      tds_va[16 * bd + 1] = (next_ptr >> 32u);      // NXTDESC_PNTR_MSB
      tds_va[16 * bd + 2] = sa & 0xffffffffu;       // SA
      tds_va[16 * bd + 3] = (sa >> 32u);            // SA_MSB
      tds_va[16 * bd + 4] = da & 0xffffffffu;       // DA
      tds_va[16 * bd + 5] = (da >> 32u);            // DA_MSB
      tds_va[16 * bd + 6] = config_header.ch_size;  // CONTROL (BTT)
      tds_va[16 * bd + 7] = 0;                      // STATUS
      bd++;
      if (next_chcfg_idx + 2 ==
          file_header.num_ch) { // FIXME save rather than "last"
        last_td = next_ptr;
        next_ptr = 0;
      } else {
        next_ptr = uint64_t(tds_pa) + 64ul * (bd + 1);
      }
      sa += config_header.ch_size;
    }
    next_chcfg_idx++;
  }

  return last_td;
}
