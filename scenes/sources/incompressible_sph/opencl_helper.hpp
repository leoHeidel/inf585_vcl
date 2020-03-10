#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
 

struct OCLHelper {
    std::string kernel_paths = "scenes/sources/incompressible_sph/kernels/";
    cl_context context;
    cl_device_id device_id = NULL;   
    cl_command_queue command_queue;

    void init_context();
    void test_context();

    ~OCLHelper();

    private:
    std::string getSource(std::string kernelName);
};