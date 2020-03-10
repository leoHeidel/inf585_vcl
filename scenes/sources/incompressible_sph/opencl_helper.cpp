#include "opencl_helper.hpp"

#include <iostream>
#include <fstream>
#include <sstream>


void OCLHelper::init_context(){
    std::cout << "Initialising OpenCL context" << std::endl;

    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    std::cout << "Error code clGetPlatformIDs : " << ret << std::endl;

    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, 
            &device_id, &ret_num_devices);
    std::cout << "Error code clGetDeviceIDs : " << ret << std::endl;

    cl_uint max_comput_unit;
    ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(max_comput_unit), &max_comput_unit, NULL);
    std::cout << "Error code clGetDeviceInfo : " << ret << std::endl;
    std::cout << "num devices " << ret_num_devices 
    << " num platforms " << ret_num_platforms 
    << " max comput unit " << max_comput_unit << std::endl;

    // Create an OpenCL context
    context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
    std::cout << "Error code clCreateContext : " << ret << std::endl;

    // Create a command queue
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    std::cout << "Error code clCreateCommandQueue : " << ret << std::endl;
}

void OCLHelper::test_context(){
    std::string sources =  getSource("vector_add_kernel.cl");
    const int LIST_SIZE = 8;
    int *A = (int*)malloc(sizeof(int)*LIST_SIZE);
    int *B = (int*)malloc(sizeof(int)*LIST_SIZE);
    for(int i = 0; i < LIST_SIZE; i++) {
        A[i] = i;
        B[i] = LIST_SIZE - i;
    }

    const char* source_str = sources.c_str();
    const size_t source_size = sources.size();

    std::cout << "kernel:" << std::endl << source_str << std::endl;

    cl_int ret;
        // Create memory buffers on the device for each vector 
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            LIST_SIZE * sizeof(int), NULL, &ret);
    std::cout << "Error code clCreateBuffer : " << ret << std::endl;
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
            LIST_SIZE * sizeof(int), NULL, &ret);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
            LIST_SIZE * sizeof(int), NULL, &ret);
    std::cout << "Error code clCreateBuffer : " << ret << std::endl;

    
    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
            LIST_SIZE * sizeof(int), A, 0, NULL, NULL);
    std::cout << "Error code clEnqueueWriteBuffer : " << ret << std::endl;
    ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(int), B, 0, NULL, NULL);
    std::cout << "Error code clEnqueueWriteBuffer : " << ret << std::endl;


    cl_program program = clCreateProgramWithSource(context, 1, 
        (const char **)&source_str, (const size_t *)&source_size, &ret);

    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    std::cout << "Error code clBuildProgram : " << clBuildProgram << std::endl;

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);
    std::cout << "Error code clCreateKernel : " << clCreateKernel << std::endl;

    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
    std::cout << "Error code clSetKernelArg : " << ret << std::endl;
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);
    std::cout << "Error code clSetKernelArg : " << ret << std::endl;

    // Execute the OpenCL kernel on the list
    size_t global_item_size = LIST_SIZE; // Process the entire lists
    size_t local_item_size = 4; // Divide work items into groups of 64
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
            &global_item_size, &local_item_size, 0, NULL, NULL);
    std::cout << "Error code clEnqueueNDRangeKernel : " << ret << std::endl;

    // Read the memory buffer C on the device to the local variable C
    int *C = (int*)malloc(sizeof(int)*LIST_SIZE);
    ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(int), C, 0, NULL, NULL);
    std::cout << "Error code clEnqueueReadBuffer : " << ret << std::endl;

    // Display the result to the screen
    for(int i = 0; i < LIST_SIZE; i++)
        printf("%d + %d = %d\n", A[i], B[i], C[i]);

    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
    ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(c_mem_obj);

    free(A);
    free(B);
    free(C);
}

std::string OCLHelper::getSource(std::string kernelName){
    std::ifstream kernelFile(kernel_paths + kernelName);
    if (kernelFile)
    {
        std::ostringstream ss;
        ss << kernelFile.rdbuf();
        return ss.str();
    }
    else {
        std::cerr <<"kernel not found at path : " << kernel_paths << kernelName;
        exit(1);
    }
}

OCLHelper::~OCLHelper(){
    cl_int ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
}