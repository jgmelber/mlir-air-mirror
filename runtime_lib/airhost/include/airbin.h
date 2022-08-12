#ifndef AIRBIN_H
#define AIRBIN_H

#include <cstdint>
#include <fstream>

uint64_t airbin2mem(std::ifstream &infile, volatile uint32_t *tds_va,
                    uint32_t *tds_pa, volatile uint32_t *data_va,
                    uint32_t *data_pa, uint8_t col);

#endif
