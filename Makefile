ARCH       := $(shell getconf LONG_BIT)
OS         := $(shell cat /etc/issue)

# Gets Driver Branch
DRIVER_BRANCH := $(shell nvidia-smi | grep Driver | cut -f 3 -d' ' | cut -f 1 -d '.')

# Location of the CUDA Toolkit
CUDA_PATH ?= "/usr/local/cuda-12.1"

ifeq (${ARCH},$(filter ${ARCH},32 64))
    # If correct architecture and libnvidia-ml library is not found 
    # within the environment, build using the stub library
    
    ifneq (,$(findstring Ubuntu,$(OS)))
        DEB := $(shell dpkg -l | grep cuda)
        ifneq (,$(findstring cuda, $(DEB)))
            NVML_LIB := /usr/lib/nvidia-$(DRIVER_BRANCH)
        else 
            NVML_LIB := /lib${ARCH}
        endif
    endif
else
    NVML_LIB := ../../lib${ARCH}/stubs/
    $(info "libnvidia-ml.so.1" not found, using stub library.)
endif

ifneq (${ARCH},$(filter ${ARCH},32 64))
	$(error Unknown architecture!)
endif

NVML_LIB += $(CUDA_PATH)/lib64/
NVML_LIB_L := $(addprefix -L , $(NVML_LIB))

CFLAGS  := -I  $(CUDA_PATH)/include
LDFLAGS := -lnvidia-ml -lcudart $(NVML_LIB_L)

all: smitool
smitool: smitool.cpp
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@
clean:
	-@rm -f smitool.o
	-@rm -f smitool