#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <string>
#include <fstream>

#include "vcl/vcl.hpp"

// SPH simulation parameters
struct sph_parameters
{
    cl_int nb_particles=8192;
    cl_int hash_table_size=4096;
    cl_int table_list_size=128;
    cl_int nb_neighbors=64;

    cl_float h = 0.06f;
    cl_float rho0 = 1000.0f;
    cl_float m; // rho0*h*h*h
    cl_float epsilon = 1e-3f;
    cl_float c = 0.2;
    cl_float dt = 0.02f;
    cl_float max_relative_dp = 0.08f;
    cl_float gx = 0.0f;
    cl_float gy = -h*100.0f;
    cl_float gz = 0.0f;
};


struct OCLHelper {
    std::string kernel_paths = "scenes/sources/incompressible_sph/kernels/";
    cl_context context;
    cl_device_id device_id = NULL;
    cl_command_queue command_queue;

    int nb_particles;
    int hash_table_size;
    int table_list_size;
    int nb_neighbors;

    size_t local_item_size = 128;

    cl_mem sph_param_mem;
    cl_mem p_mem;
    cl_mem table_mem;
    cl_mem table_count_mem;
    cl_mem neighbors_mem;
    cl_mem n_neighbors_mem;
    cl_mem q_mem;
    cl_mem lambda_mem;
    cl_mem dp_mem;
    cl_mem v_mem;
    cl_mem v_copy_mem;
    cl_mem w_mem;
    cl_mem pressure_mem;

    cl_program hashmap_program;
    cl_program solver_program;
    cl_program speed_program;

    cl_kernel fill_hashmap_kernel;
    cl_kernel find_neighbors_kernel;

    cl_kernel compute_constraints_kernel;
    cl_kernel compute_dp_kernel;
    cl_kernel solve_collisions_kernel;
    cl_kernel add_position_correction_kernel;

    cl_kernel befor_solver_kernel;
    cl_kernel update_position_speed_kernel;
    cl_kernel update_w_kernel;
    cl_kernel apply_vorticity_kernel;
    cl_kernel apply_viscosity_kernel;
    cl_kernel compute_pressure_kernel;

    float alpha_time = 0.6;
    float nn1_time;
    float nn2_time;
    float nn3_time;

    std::ofstream pressure_log_file;

    void init_context(sph_parameters sph_param);

    void set_sph_param(sph_parameters sph_param);
    void set_p_v(std::vector<vcl::vec3> positions, std::vector<vcl::vec3> v);
    void befor_solver();
    std::vector<vcl::vec3> get_v();
    std::vector<vcl::vec3> get_p();
    void make_neighboors();
    void solver_step();
    void update_speed();
    void log_pressure();

    ~OCLHelper();

    private:
    void init_buffers();
    void init_hashmap_program();
    void init_solver_program();
    void init_speed_program();

    cl_program load_source(std::string kernelName);
};
