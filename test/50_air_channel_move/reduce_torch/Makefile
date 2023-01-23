# Copyright (C) 2022, Xilinx Inc.
# Copyright (C) 2022, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: MIT

include ../config.make

all: reduce.channel.mlir #main.exe

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

%.exe: %.o test_library.o reduce.air.a
	$(CC) $(LDFLAGS) -o $@ $< test_library.o -Wl,--whole-archive reduce.air.a -Wl,--no-whole-archive -lm

reduce.mlir: reduce.py
	python3 $< > $@

# change linalg on tensors into linalg on memrefs
reduce.linalg-memrefs.mlir: reduce.mlir
	torch-mlir-opt \
		--refback-mlprogram-bufferize \
		--linalg-bufferize --cse \
		--func-bufferize \
		--tensor-bufferize \
		--finalizing-bufferize \
		-o $@ $<

reduce.air.mlir: reduce.linalg-memrefs.mlir
	$(AIR_OPT) -o $@ $< \
			-buffer-results-to-out-params \
			-air-linalg-codegen='l1-tile-size=32,32,32' \
			-air-par-to-herd -air-copy-to-dma \
			-canonicalize -cse \
			-convert-linalg-to-affine-loops

reduce.channel.mlir: reduce.air.mlir
	$(AIR_OPT) -o $@ $< \
			-air-dma-to-channel 
			#-canonicalize -cse 

reduce.air.a: reduce.air.mlir
	aircc.py -o $@ --sysroot=${SYSROOT} $<

clean::
	rm -rf air_project reduce.*mlir* *.elf *.exe *.o *.a
