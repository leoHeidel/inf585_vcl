#include "opencl_helper.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono> 


using namespace vcl;

void OCLHelper::init_context(sph_parameters sph_param){
    nb_particles=sph_param.nb_particles;
    hash_table_size=sph_param.hash_table_size;
    table_list_size=sph_param.table_list_size;
    nb_neighbors=sph_param.nb_neighbors;

    std::cout << "Initialising OpenCL context" << std::endl;

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

    context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

    init_buffers();
    init_hashmap_program();
    init_solver_program();
    init_speed_program();
    set_sph_param(sph_param);

    pressure_log_file.open("pressure_log.csv");
}

void OCLHelper::init_buffers(){
    cl_int ret;
    sph_param_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(sph_parameters), NULL, &ret);
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
    w_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, nb_particles * sizeof(cl_float3), NULL, &ret);
    pressure_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, nb_particles * sizeof(cl_float), NULL, &ret);
 }


void OCLHelper::init_hashmap_program(){

    hashmap_program =  load_source("hashmap_kernel.cl");

    cl_int ret;
    fill_hashmap_kernel = clCreateKernel(hashmap_program, "fill_hashmap", &ret);
    find_neighbors_kernel = clCreateKernel(hashmap_program, "find_neighbors", &ret);

    ret = clSetKernelArg(fill_hashmap_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(fill_hashmap_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(fill_hashmap_kernel, 2, sizeof(cl_mem), (void *)&table_mem);
    ret = clSetKernelArg(fill_hashmap_kernel, 3, sizeof(cl_mem), (void *)&table_count_mem);

    ret = clSetKernelArg(find_neighbors_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 2, sizeof(cl_mem), (void *)&table_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 3, sizeof(cl_mem), (void *)&table_count_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 4, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(find_neighbors_kernel, 5, sizeof(cl_mem), (void *)&n_neighbors_mem);
}

void OCLHelper::init_solver_program(){
    solver_program =  load_source("solver_kernels.cl");

    cl_int ret;
    compute_constraints_kernel = clCreateKernel(solver_program, "compute_constraints", &ret);
    compute_dp_kernel = clCreateKernel(solver_program, "compute_dp", &ret);
    solve_collisions_kernel = clCreateKernel(solver_program, "solve_collisions", &ret);
    add_position_correction_kernel = clCreateKernel(solver_program, "add_position_correction", &ret);

    ret = clSetKernelArg(compute_constraints_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 1, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 2, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 3, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(compute_constraints_kernel, 4, sizeof(cl_mem), (void *)&lambda_mem);

    ret = clSetKernelArg(compute_dp_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(compute_dp_kernel, 1, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(compute_dp_kernel, 2, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(compute_dp_kernel, 3, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(compute_dp_kernel, 4, sizeof(cl_mem), (void *)&lambda_mem);
    ret = clSetKernelArg(compute_dp_kernel, 5, sizeof(cl_mem), (void *)&dp_mem);

    ret = clSetKernelArg(solve_collisions_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(solve_collisions_kernel, 1, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(solve_collisions_kernel, 2, sizeof(cl_mem), (void *)&dp_mem);

    ret = clSetKernelArg(add_position_correction_kernel, 0, sizeof(cl_mem), (void *)&dp_mem);
    ret = clSetKernelArg(add_position_correction_kernel, 1, sizeof(cl_mem), (void *)&q_mem);
}

void OCLHelper::init_speed_program(){
    speed_program =  load_source("update_speed_kernels.cl");

    cl_int ret;
    befor_solver_kernel = clCreateKernel(speed_program, "befor_solver", &ret);
    update_position_speed_kernel = clCreateKernel(speed_program, "update_position_speed", &ret);
    update_w_kernel = clCreateKernel(speed_program, "update_w", &ret);
    apply_vorticity_kernel = clCreateKernel(speed_program, "apply_vorticity", &ret);
    apply_viscosity_kernel = clCreateKernel(speed_program, "apply_viscosity", &ret);
    compute_pressure_kernel = clCreateKernel(speed_program, "compute_pressure", &ret);

    // Set the arguments of the kernels
    ret = clSetKernelArg(befor_solver_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(befor_solver_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(befor_solver_kernel, 2, sizeof(cl_mem), (void *)&v_mem);
    ret = clSetKernelArg(befor_solver_kernel, 3, sizeof(cl_mem), (void *)&q_mem);

    ret = clSetKernelArg(update_position_speed_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(update_position_speed_kernel, 1, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(update_position_speed_kernel, 2, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(update_position_speed_kernel, 3, sizeof(cl_mem), (void *)&v_copy_mem);

    ret = clSetKernelArg(update_w_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(update_w_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(update_w_kernel, 2, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(update_w_kernel, 3, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(update_w_kernel, 4, sizeof(cl_mem), (void *)&v_copy_mem);
    ret = clSetKernelArg(update_w_kernel, 5, sizeof(cl_mem), (void *)&w_mem);

    ret = clSetKernelArg(apply_vorticity_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(apply_vorticity_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(apply_vorticity_kernel, 2, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(apply_vorticity_kernel, 3, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(apply_vorticity_kernel, 4, sizeof(cl_mem), (void *)&v_copy_mem);
    ret = clSetKernelArg(apply_vorticity_kernel, 5, sizeof(cl_mem), (void *)&w_mem);

    ret = clSetKernelArg(apply_viscosity_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 1, sizeof(cl_mem), (void *)&q_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 2, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 3, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 4, sizeof(cl_mem), (void *)&v_copy_mem);
    ret = clSetKernelArg(apply_viscosity_kernel, 5, sizeof(cl_mem), (void *)&v_mem);
    
    ret = clSetKernelArg(compute_pressure_kernel, 0, sizeof(cl_mem), (void *)&sph_param_mem);
    ret = clSetKernelArg(compute_pressure_kernel, 1, sizeof(cl_mem), (void *)&p_mem);
    ret = clSetKernelArg(compute_pressure_kernel, 2, sizeof(cl_mem), (void *)&neighbors_mem);
    ret = clSetKernelArg(compute_pressure_kernel, 3, sizeof(cl_mem), (void *)&n_neighbors_mem);
    ret = clSetKernelArg(compute_pressure_kernel, 4, sizeof(cl_mem), (void *)&pressure_mem);
}

cl_program OCLHelper::load_source(std::string kernelName){
    std::ifstream kernelFile(kernel_paths + kernelName);
    if (!kernelFile)
    {
        std::cerr <<"kernel not found at path : " << kernel_paths << kernelName;
        exit(1);
    }
    std::ostringstream ss;
    ss << kernelFile.rdbuf();
    std::string sources = ss.str();
    const char* source_str = sources.c_str();
    const size_t source_size = sources.size();
    cl_int ret;
    cl_program program = clCreateProgramWithSource(context, 1,
        (const char **)&source_str, (const size_t *)&source_size, &ret);
    std::cout << "Error code clCreateProgramWithSource : " << ret << std::endl;
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if(ret != CL_SUCCESS)
    {
        std::cout<<"Program Build failed\n";
        size_t length;
        char buffer[16192];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        std::cout<<"--- Build log ---\n "<<buffer<<std::endl;
        exit(1);
    }
    return program;
}


void OCLHelper::set_sph_param(sph_parameters sph_param){
    cl_int ret;
    nb_particles=sph_param.nb_particles;
    hash_table_size=sph_param.hash_table_size;
    table_list_size=sph_param.table_list_size;
    nb_neighbors=sph_param.nb_neighbors;
    ret = clEnqueueWriteBuffer(command_queue, sph_param_mem, CL_TRUE, 0,  sizeof(sph_param), &sph_param, 0, NULL, NULL);
}


void OCLHelper::make_neighboors(){
    auto t1 = std::chrono::high_resolution_clock::now();
    
    cl_int zero = 0;
    cl_int ret = clEnqueueFillBuffer(command_queue, table_count_mem, &zero, sizeof(zero), 0, sizeof(cl_int) * hash_table_size, 0, NULL, NULL);
    ret = clEnqueueFillBuffer(command_queue, n_neighbors_mem, &zero, sizeof(zero), 0, sizeof(cl_int) * nb_particles, 0, NULL, NULL);

    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);

    size_t global_item_size = nb_particles;
    cl_event  last_kernel;

    clFinish(command_queue);
    auto t2 = std::chrono::high_resolution_clock::now();


    ret = clEnqueueNDRangeKernel(command_queue, fill_hashmap_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &barrier, &last_kernel);

    clFinish(command_queue);
    auto t3 = std::chrono::high_resolution_clock::now();


    ret = clEnqueueNDRangeKernel(command_queue, find_neighbors_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel, NULL);
    

    clFinish(command_queue);
    auto t4 = std::chrono::high_resolution_clock::now();

    nn1_time = alpha_time*nn1_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count();
    nn2_time = alpha_time*nn2_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(t3-t2).count();
    nn3_time = alpha_time*nn3_time + (1-alpha_time)*std::chrono::duration_cast<std::chrono::milliseconds>(t4-t3).count();
}

void OCLHelper::solver_step(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    size_t global_item_size = nb_particles;
    cl_event  last_kernel;
    ret = clEnqueueNDRangeKernel(command_queue, compute_constraints_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &barrier, &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, compute_dp_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, solve_collisions_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, add_position_correction_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    

    clFinish(command_queue);
}

void OCLHelper::update_speed(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    size_t global_item_size = nb_particles;
    cl_event  last_kernel;

    ret = clEnqueueNDRangeKernel(command_queue, update_position_speed_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &barrier, &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, update_w_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, apply_vorticity_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
    ret = clEnqueueNDRangeKernel(command_queue, apply_viscosity_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &last_kernel,  &last_kernel);
}

std::vector<vcl::vec3> OCLHelper::get_v(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    cl_float3 *result = (cl_float3*)malloc(sizeof(cl_float3) * nb_particles);
    ret = clEnqueueReadBuffer(command_queue, v_mem, CL_TRUE, 0,
            sizeof(cl_float3) * nb_particles, result, 0, NULL, NULL);
    std::vector<vcl::vec3> res;
    for (size_t i = 0; i < nb_particles; i++)
    {
        res.push_back(vcl::vec3({result[i].s[0],result[i].s[1],result[i].s[2]}));
    }
    free(result);
    return res;
}


std::vector<vcl::vec3> OCLHelper::get_p(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    cl_float3 *result = (cl_float3*)malloc(sizeof(cl_float3) * nb_particles);
    ret = clEnqueueReadBuffer(command_queue, p_mem, CL_TRUE, 0,
            sizeof(cl_float3) * nb_particles, result, 0, NULL, NULL);
    std::vector<vcl::vec3> res;
    for (size_t i = 0; i < nb_particles; i++)
    {
        res.push_back(vcl::vec3({result[i].s[0],result[i].s[1],result[i].s[2]}));
    }
    free(result);
    return res;
}


void OCLHelper::set_p_v(std::vector<vec3> positions, std::vector<vec3> v){
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
    free (v_array);
    free(positions_array);
}

void OCLHelper::befor_solver(){
    cl_event  barrier;
    cl_int ret;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    size_t global_item_size = nb_particles;
    ret = clEnqueueNDRangeKernel(command_queue, befor_solver_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &barrier, NULL);
    clFinish(command_queue);
}

void OCLHelper::log_pressure(){
    cl_int ret;
    cl_event  barrier;
    ret = clEnqueueBarrierWithWaitList(command_queue, 0, NULL, &barrier);
    cl_float *result = (cl_float*)malloc(sizeof(cl_float) * nb_particles);
    size_t global_item_size = nb_particles;
    ret = clEnqueueNDRangeKernel(command_queue, compute_pressure_kernel, 1, NULL,
            &global_item_size, &local_item_size, 1, &barrier, &barrier);
    ret = clEnqueueReadBuffer(command_queue, pressure_mem, CL_TRUE, 0,
            sizeof(cl_float) * nb_particles, result, 1, &barrier, NULL);
    for (size_t i = 0; i < nb_particles; i++)
    {
        pressure_log_file << result[i] << ",";
    }
    pressure_log_file << std::endl;
    free(result);
}


OCLHelper::~OCLHelper(){
    pressure_log_file.close();

    cl_int ret;

    ret = clReleaseKernel(fill_hashmap_kernel);
    ret = clReleaseKernel(find_neighbors_kernel);

    ret = clReleaseKernel(compute_constraints_kernel);
    ret = clReleaseKernel(compute_dp_kernel);
    ret = clReleaseKernel(solve_collisions_kernel);
    ret = clReleaseKernel(add_position_correction_kernel);

    ret = clReleaseKernel(befor_solver_kernel);
    ret = clReleaseKernel(update_position_speed_kernel);
    ret = clReleaseKernel(apply_viscosity_kernel);
    ret = clReleaseKernel(update_w_kernel);
    ret = clReleaseKernel(apply_vorticity_kernel);
    ret = clReleaseKernel(compute_pressure_kernel);

    ret = clReleaseProgram(hashmap_program);
    ret = clReleaseProgram(solver_program);
    ret = clReleaseProgram(speed_program);

    ret = clReleaseMemObject(sph_param_mem);
    ret = clReleaseMemObject(p_mem);
    ret = clReleaseMemObject(table_mem);
    ret = clReleaseMemObject(table_count_mem);
    ret = clReleaseMemObject(neighbors_mem);
    ret = clReleaseMemObject(n_neighbors_mem);
    ret = clReleaseMemObject(q_mem);
    ret = clReleaseMemObject(lambda_mem);
    ret = clReleaseMemObject(dp_mem);
    ret = clReleaseMemObject(v_mem);
    ret = clReleaseMemObject(v_copy_mem);
    ret = clReleaseMemObject(w_mem);
    ret = clReleaseMemObject(pressure_mem);

    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
}
