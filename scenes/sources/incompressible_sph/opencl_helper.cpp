#include "opencl_helper.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace vcl;

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
    char name[64];
    ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(char)*64, name, NULL);
    std::cout << "Error code clGetDeviceInfo : " << ret << std::endl;

    std::cout << "num devices " << ret_num_devices 
    << " num platforms " << ret_num_platforms 
    << " max comput unit " << max_comput_unit 
    << " name: " << std::string(name) << std::endl;

    // Create an OpenCL context
    context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
    // Create a command queue
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    init_buffers();
}

void OCLHelper::init_buffers(){
    cl_int ret;

    p_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, nb_particles * sizeof(cl_float3), NULL, &ret);
    table_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, hash_table_size * table_list_size * sizeof(cl_int), NULL, &ret);
    table_count_mem = clCreateBuffer(context, CL_MEM_READ_WRITE,  hash_table_size * sizeof(cl_int), NULL, &ret);
    neighbors_mem = clCreateBuffer(context, CL_MEM_READ_WRITE,  nb_particles * nb_neighbors * sizeof(cl_int), NULL, &ret);
    n_neighbors_mem = clCreateBuffer(context, CL_MEM_READ_WRITE,  nb_particles * sizeof(cl_int), NULL, &ret);
    q_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nb_particles * sizeof(cl_float3), NULL, &ret);
    lambda_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nb_particles * sizeof(cl_float), NULL, &ret);
    dp_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nb_particles * sizeof(cl_float3), NULL, &ret);
    v_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nb_particles * sizeof(cl_float3), NULL, &ret);
    v_copy_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nb_particles * sizeof(cl_float3), NULL, &ret);


    //Init the kernels for neighbors search

    std::string sources =  getSource("hashmap_kernel.cl");
    const char* source_str = sources.c_str();
    const size_t source_size = sources.size();
    hashmap_program = clCreateProgramWithSource(context, 1, 
        (const char **)&source_str, (const size_t *)&source_size, &ret);
    std::cout << "Error code clCreateProgramWithSource : " << ret << std::endl;
    ret = clBuildProgram(hashmap_program, 1, &device_id, NULL, NULL, NULL);
    if(ret != CL_SUCCESS)
    {
        std::cout<<"Program Build failed\n";
        size_t length;
        char buffer[2048*2*2];
        clGetProgramBuildInfo(hashmap_program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        std::cout<<"--- Build log ---\n "<<buffer<<std::endl;
        exit(1);
    }
    fill_hashmap_kernel = clCreateKernel(hashmap_program, "fill_hashmap", &ret);
    find_neighbors_kernel = clCreateKernel(hashmap_program, "find_neighbors", &ret);

    // Set the arguments of the kernels
    ret = clSetKernelArg(fill_hashmap_kernel, 0, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(fill_hashmap_kernel, 1, sizeof(cl_mem), (void *)&table_mem);
    ret = clSetKernelArg(fill_hashmap_kernel, 2, sizeof(cl_mem), (void *)&table_count_mem);

    ret = clSetKernelArg(find_neighbors_kernel, 0, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 1, sizeof(cl_mem), (void *)&table_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 2, sizeof(cl_mem), (void *)&table_count_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 3, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 4, sizeof(cl_mem), (void *)&n_neighbors_mem);

    //Init the kernels for the solver
    std::string sources_solver =  getSource("solver_kernels.cl");
    const char* source_str_solver = sources_solver.c_str();
    const size_t source_size_solver = sources_solver.size();
    solver_program = clCreateProgramWithSource(context, 1, 
        (const char **)&source_str_solver, (const size_t *)&source_size_solver, &ret);
    std::cout << "Error code clCreateProgramWithSource : " << ret << std::endl;
    ret = clBuildProgram(solver_program, 1, &device_id, NULL, NULL, NULL);
    if(ret != CL_SUCCESS)
    {
        std::cout<<"Program Build failed\n";
        size_t length;
        char buffer[2048*2*2];
        clGetProgramBuildInfo(solver_program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        std::cout<<"--- Build log ---\n "<<buffer<<std::endl;
        exit(1);
    }
    compute_constraints_kernel = clCreateKernel(solver_program, "compute_constraints", &ret);
    compute_dp_kernel = clCreateKernel(solver_program, "compute_dp", &ret);
    solve_collisions_kernel = clCreateKernel(solver_program, "solve_collisions", &ret);
    add_position_correction_kernel = clCreateKernel(solver_program, "add_position_correction", &ret);
    
    // Set the arguments of the kernels
    ret = clSetKernelArg(compute_constraints_kernel, 0, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 1, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 2, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 3, sizeof(cl_mem), (void *)&lambda_mem);

    ret = clSetKernelArg(compute_dp_kernel, 0, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(compute_dp_kernel, 1, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(compute_dp_kernel, 2, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(compute_dp_kernel, 3, sizeof(cl_mem), (void *)&lambda_mem);
    ret = clSetKernelArg(compute_dp_kernel, 4, sizeof(cl_mem), (void *)&dp_mem);

    ret = clSetKernelArg(solve_collisions_kernel, 0, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(solve_collisions_kernel, 1, sizeof(cl_mem), (void *)&dp_mem);
    
    ret = clSetKernelArg(add_position_correction_kernel, 0, sizeof(cl_mem), (void *)&dp_mem);
    ret = clSetKernelArg(add_position_correction_kernel, 1, sizeof(cl_mem), (void *)&q_mem);


    //Init the kernels for the speed
    std::string sources_speed =  getSource("update_speed_kernels.cl");
    const char* source_str_speed = sources_speed.c_str();
    const size_t source_size_speed = sources_speed.size();
    speed_program = clCreateProgramWithSource(context, 1, 
        (const char **)&source_str_speed, (const size_t *)&source_size_speed, &ret);
    std::cout << "Error code clCreateProgramWithSource : " << ret << std::endl;
    ret = clBuildProgram(speed_program, 1, &device_id, NULL, NULL, NULL);
    if(ret != CL_SUCCESS)
    {
        std::cout<<"Program Build failed\n";
        size_t length;
        char buffer[2048*2*2];
        clGetProgramBuildInfo(speed_program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        std::cout<<"--- Build log ---\n "<<buffer<<std::endl;
        exit(1);
    }
    befor_solver_kernel = clCreateKernel(speed_program, "befor_solver", &ret);
    update_position_speed_kernel = clCreateKernel(speed_program, "update_position_speed", &ret);
    apply_viscosity_kernel = clCreateKernel(speed_program, "apply_viscosity", &ret);

    // Set the arguments of the kernels
    ret = clSetKernelArg(befor_solver_kernel, 0, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(befor_solver_kernel, 1, sizeof(cl_mem), (void *)&v_mem);
    ret = clSetKernelArg(befor_solver_kernel, 2, sizeof(cl_mem), (void *)&q_mem);

    ret = clSetKernelArg(update_position_speed_kernel, 0, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(update_position_speed_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(update_position_speed_kernel, 2, sizeof(cl_mem), (void *)&v_copy_mem);

    ret = clSetKernelArg(apply_viscosity_kernel, 0, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 1, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 2, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 3, sizeof(cl_mem), (void *)&v_copy_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 4, sizeof(cl_mem), (void *)&v_mem);
}

void OCLHelper::make_neighboors(){
      cl_int zero = 0;
    cl_int ret = clEnqueueFillBuffer(command_queue, table_count_mem, &zero, sizeof(zero), 0, sizeof(cl_int) * hash_table_size, 0, NULL, NULL);
    ret = clEnqueueFillBuffer(command_queue, n_neighbors_mem, &zero, sizeof(zero), 0, sizeof(cl_int) * nb_particles, 0, NULL, NULL);
    
    // cl_int neg = -1;
    // ret = clEnqueueFillBuffer(command_queue, neighbors_mem, &neg, sizeof(neg), 0, sizeof(cl_int) * nb_particles * nb_neighbors, 0, NULL, NULL);
    // cl_int neg2 = -2;
    // ret = clEnqueueFillBuffer(command_queue, table_mem, &neg2, sizeof(neg2), 0, sizeof(cl_int) * table_list_size * hash_table_size, 0, NULL, NULL);

    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);

    // Execute the OpenCL kernel on the list
    size_t global_item_size = nb_particles;
    size_t local_item_size = 4; 
    cl_event  last_kernel;
    ret = clEnqueueNDRangeKernel(command_queue, fill_hashmap_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &barrier, &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, find_neighbors_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &last_kernel, NULL);

    // ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);

    // cl_int *result = (cl_int*)malloc(sizeof(cl_int) * hash_table_size* table_list_size);
    // ret = clEnqueueReadBuffer(command_queue, table_mem, CL_TRUE, 0, 
    //         sizeof(cl_int) * hash_table_size* table_list_size, result, 0, NULL, NULL);

    // cl_int *neighboors_found = (cl_int*)malloc(sizeof(cl_int) * nb_neighbors* nb_particles);
    // ret = clEnqueueReadBuffer(command_queue, neighbors_mem, CL_TRUE, 0, 
    //         sizeof(cl_int) * nb_neighbors * nb_particles, neighboors_found, 0, NULL, NULL);

    // std::cout << "hashtable state" << std::endl;
    // for (size_t i = 0; i < hash_table_size; i++)
    // {
    //    for (size_t j = 0; j < table_list_size; j++)
    //    {
    //        std::cout << result[i*table_list_size + j] << " ";
    //    }
    //    std::cout << std::endl;
    // }
    // std::cout << "neighboors found" << std::endl;
    // for (size_t i = 0; i < nb_particles; i++)
    // {
    //    for (size_t j = 0; j < nb_neighbors; j++)
    //    {
    //        std::cout << neighboors_found[i*nb_neighbors + j] << " ";
    //    }
    //    std::cout << std::endl;
    // }
    // std::cout << std::endl;

    // free(result);
    // free(neighboors_found);
}

void OCLHelper::solver_step(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    size_t global_item_size = nb_particles;
    size_t local_item_size = 4; 
    cl_event  last_kernel;
    ret = clEnqueueNDRangeKernel(command_queue, compute_constraints_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &barrier, &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, compute_dp_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, solve_collisions_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, add_position_correction_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);





    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    cl_float3 *result = (cl_float3*)malloc(sizeof(cl_float3) * nb_particles);
    ret = clEnqueueReadBuffer(command_queue, q_mem, CL_TRUE, 0, 
            sizeof(cl_float3) * nb_particles, result, 0, NULL, NULL);

    std::cout << "q GPU";
    for (size_t i = 0; i < 3; i++)
    {
       for (size_t j = 0; j < 3; j++)
       {
           std::cout << result[i].s[j] << " ";
       }
       std::cout << "   ";
    }
    std::cout << std::endl;
    free(result);
}

void OCLHelper::update_speed(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    size_t global_item_size = nb_particles;
    size_t local_item_size = 4; 
    cl_event  last_kernel;

    ret = clEnqueueNDRangeKernel(command_queue, update_position_speed_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &barrier, &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, apply_viscosity_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);




    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    cl_float3 *result = (cl_float3*)malloc(sizeof(cl_float3) * nb_particles);
    ret = clEnqueueReadBuffer(command_queue, v_mem, CL_TRUE, 0, 
            sizeof(cl_float3) * nb_particles, result, 0, NULL, NULL);

    std::cout << "v GPU";
    for (size_t i = 0; i < 3; i++)
    {
       for (size_t j = 0; j < 3; j++)
       {
           std::cout << result[i].s[j] << " ";
       }
       std::cout << "   ";
    }
    std::cout << std::endl;
    free(result);
    
}

void OCLHelper::befor_solver(std::vector<vec3> positions, std::vector<vec3> v){
    cl_float3* positions_array = (cl_float3*)malloc(sizeof(cl_float3)*nb_particles);
    for (int i = 0; i < nb_particles; i++)
    {
        positions_array[i].s[0] = positions[i].x;
        positions_array[i].s[1] = positions[i].y;
        positions_array[i].s[2] = positions[i].z;
    }
    cl_int ret;
    ret = clEnqueueWriteBuffer(command_queue, p_mem, CL_TRUE, 0, nb_particles * sizeof(cl_float3), positions_array, 0, NULL, NULL);

    
    cl_float3* v_array = (cl_float3*)malloc(sizeof(cl_float3)*nb_particles);
    for (int i = 0; i < nb_particles; i++)
    {
        v_array[i].s[0] = v[i].x;
        v_array[i].s[1] = v[i].y;
        v_array[i].s[2] = v[i].z;
    }
    ret = clEnqueueWriteBuffer(command_queue, v_mem, CL_TRUE, 0, nb_particles * sizeof(cl_float3), v_array, 0, NULL, NULL);
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    size_t global_item_size = nb_particles;
    size_t local_item_size = 4; 
    ret = clEnqueueNDRangeKernel(command_queue, befor_solver_kernel, 1, NULL, 
            &global_item_size, &local_item_size, 1, &barrier, NULL);


    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    cl_float3 *result = (cl_float3*)malloc(sizeof(cl_float3) * nb_particles);
    ret = clEnqueueReadBuffer(command_queue, q_mem, CL_TRUE, 0, 
            sizeof(cl_float3) * nb_particles, result, 0, NULL, NULL);

    std::cout << "befor solver GPU";
    for (size_t i = 0; i < 3; i++)
    {
       for (size_t j = 0; j < 3; j++)
       {
           std::cout << result[i].s[j] << " ";
       }
       std::cout << "   ";
    }
    std::cout << std::endl;
    free(result);
    free (v_array);
    free(positions_array);

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
    cl_int ret;

    ret = clReleaseKernel(fill_hashmap_kernel);
    ret = clReleaseKernel(find_neighbors_kernel);
    ret = clReleaseProgram(hashmap_program);

    ret = clReleaseMemObject(p_mem);
    ret = clReleaseMemObject(table_mem);
    ret = clReleaseMemObject(table_count_mem);
    ret = clReleaseMemObject(neighbors_mem);
    ret = clReleaseMemObject(n_neighbors_mem);


    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
}