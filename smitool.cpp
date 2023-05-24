#include <stdio.h>

#include <cuda_runtime.h>
#include <nvml.h>

#ifdef _WIN32
#include <Windows.h>

#else
#include <time.h>

static void Sleep(int milliseconds)
{
    timespec req = { 0 };
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
    nanosleep(&req, 0);
}

#endif

/*
* 'index', 'utilization.gpu', 'utilization.memory', 'memory.total', 'memory.free', 'memory.used',

'temperature.gpu', 'temperature.memory', 'pstate', 'power.draw', 'enforced.power.limit',

'clocks.current.sm', 'clocks_throttle_reasons.active', 'ecc.errors.uncorrected.aggregate.total',

'ecc.errors.corrected.aggregate.total'
*/
static int GetInfo(int index)
{
    nvmlReturn_t result = NVML_SUCCESS;
    nvmlUtilization_t utilization = {0};
    nvmlPstates_t pstate = NVML_PSTATE_0;
    nvmlDevice_t device = NULL;
    nvmlMemory_t memory = {0};
    nvmlFieldValue_t memoryTempFieldValueArray[1] = { 0 };
    memoryTempFieldValueArray[0].fieldId = NVML_FI_DEV_MEMORY_TEMP;
    unsigned long long clocksThrottleReasons = 0;
    unsigned long long eccCorrectedCounts = 0;
    unsigned long long eccUnCorrectedCounts = 0;
    unsigned int clocksCount = 0;
    unsigned int GPUTemperature = 0;
    unsigned int memoryTemperature = 0;
    unsigned int power = 0;
    unsigned int powerLimit = 0;
    unsigned int clock = 0;

    result = nvmlDeviceGetHandleByIndex(index, &device);
    if (NVML_SUCCESS != result)
    {
        fprintf(stderr, "Failed to get handle for device %i: %s\n", index, nvmlErrorString(result));
        goto Error;
    }

    // temperature.gpu
    result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &GPUTemperature);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get temperature of device %i: %s\n", index, nvmlErrorString(result));
    }

    // temperature.memory
    result = nvmlDeviceGetFieldValues(device, 1, memoryTempFieldValueArray);
    if (NVML_SUCCESS != result || NVML_SUCCESS != memoryTempFieldValueArray[0].nvmlReturn)
    {
        fprintf(stderr, "Failed to get memory temperature %i: %s\n", index, 
            NVML_SUCCESS == result ? nvmlErrorString(memoryTempFieldValueArray[0].nvmlReturn) : nvmlErrorString(result));
    }
    else {
        memoryTemperature = memoryTempFieldValueArray[0].value.uiVal;
    }

    // power.draw
    result = nvmlDeviceGetPowerUsage(device, &power);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get power of device %i: %s\n", index, nvmlErrorString(result));
    }
    
    result = nvmlDeviceGetUtilizationRates(device, &utilization);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get utilization rate of device %i: %s\n", index, nvmlErrorString(result));
    }
    
    result = nvmlDeviceGetPerformanceState(device, &pstate);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get performance state of device %i: %s\n", index, nvmlErrorString(result));
    }

    result = nvmlDeviceGetMemoryInfo(device, &memory);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get memory info of device %i: %s\n", index, nvmlErrorString(result));
    }

    result = nvmlDeviceGetEnforcedPowerLimit(device, &powerLimit);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get power limit of device %i: %s\n", index, nvmlErrorString(result));
    }

    result = nvmlDeviceGetClockInfo(device, NVML_CLOCK_SM, &clock);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get clock for device %i: %s\n", index, nvmlErrorString(result));
    }

    result = nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_UNCORRECTED, NVML_VOLATILE_ECC, &eccUnCorrectedCounts);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get Ecc Error counts for device %i: %s\n", index, nvmlErrorString(result));
    }

    result = nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_CORRECTED, NVML_VOLATILE_ECC, &eccCorrectedCounts);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get Ecc Error counts for device %i: %s\n", index, nvmlErrorString(result));
    }

    result = nvmlDeviceGetCurrentClocksThrottleReasons(device, &clocksThrottleReasons);
    if (NVML_SUCCESS != result) {
        fprintf(stderr, "Failed to get throttle clock reasons for device %i: %s\n", index, nvmlErrorString(result));
    }

    printf("{ \"index\":%u, \"utilization.gpu\":%u, \"utilization.memory\":%u, \"memory.total\":%llu, \"memory.free\":%llu, \"memory.used\":%llu, \
\"temperature.gpu\":%u, \"temperature.memory\":%u, \"pstate\":%u, \"power.draw\":%u, \"enforced.power.limit\":%u, \
\"clocks.current.sm\":%u, \"clocks_throttle_reasons.active\":%llu, \"ecc.errors.uncorrected.aggregate.total\":%llu, \"ecc.errors.corrected.aggregate.total\":%llu}\n",
index, utilization.gpu, utilization.memory, memory.total, memory.free, memory.used,
GPUTemperature, memoryTemperature, pstate, power, powerLimit,
clock, clocksThrottleReasons, eccUnCorrectedCounts, eccCorrectedCounts);

Cleanup:  
    return 0;

Error:
    goto Cleanup;
}

int main(int argc, char** argv)
{
    printf("Looking for CUDA devices");

    int driverVersion = 0, runtimeVersion = 0;
    int deviceCount = 0;
       
    nvmlReturn_t result = NVML_SUCCESS;

    cudaError_t error_id = cudaGetDeviceCount(&deviceCount);

    if (error_id != cudaSuccess) {
        printf("cudaGetDeviceCount returned %d\n-> %s\n",
            (int)(error_id), cudaGetErrorString(error_id));

       return (int)error_id;
    }

    // This function call returns 0 if there are no CUDA capable devices.
    if (deviceCount == 0) {
        printf("There are no available device(s) that support CUDA\n");
    }
    else {
        printf("Detected %d CUDA Capable device(s)\n", deviceCount);
    }

    // We found devices!
     // First initialize NVML library
    result = nvmlInit();
    if (NVML_SUCCESS != result) {
        printf("Failed to initialize NVML: %s\n", nvmlErrorString(result));
        goto Error;
    }
    
    // Foreach device, print name and info
    for (int dev = 0; dev < deviceCount; ++dev) {
        cudaSetDevice(dev);
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, dev);

        printf("\n");
        printf("Device %d: %s PCI Bus ID: %d PCI Device ID: %d\n", dev, deviceProp.name, deviceProp.pciBusID, deviceProp.pciDeviceID);
        printf("\tTotal Memory %zu MB\n", deviceProp.totalGlobalMem / 1024 / 1024);
        printf("\tECC Enabled %s\n", deviceProp.ECCEnabled ? "ON" : "OFF");

        printf("Device Number: %d\n", dev);
        printf("  Device name: %s\n", deviceProp.name);
        printf("  Memory Clock Rate (MHz): %d\n",
            deviceProp.memoryClockRate / 1000);
        printf("  Memory Bus Width (bits): %d\n",
            deviceProp.memoryBusWidth);
        printf("  Peak Memory Bandwidth (GB/s): %f\n\n",
            2.0 * deviceProp.memoryClockRate * (deviceProp.memoryBusWidth / 8) / 1.0e6);
    }

    // print this until someone presses Ctrl-C
    for (;;)  {
        for (int dev = 0; dev < deviceCount; dev++) {
            GetInfo(dev);
        }
        Sleep(500);
    }

Cleanup:

    // Best effort
    result = nvmlShutdown();
    if (NVML_SUCCESS != result)
        printf("Failed to shutdown NVML: %s\n", nvmlErrorString(result));

    return 0;

Error:
    goto Cleanup;
}
