VIVADO=$(XILINX_VIVADO)/bin/vivado

all: hw

hw:
	$(VIVADO) -mode batch -notrace -source qdma_bd.tcl

clean:
	$(RM) -rf vivado.* vitis* xrc.* myproj *.backup.* NOC_Power.xpe .Xil *.str Work

ultraclean: clean
	$(RM) -rf $(PLATFORM).xsa $(PLATFORM)_presynth.pdi
