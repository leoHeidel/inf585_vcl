struct sph_parameters {
    int nb_particles;
    int hash_table_size;
    int table_list_size;
    int nb_neighbors;

    float h;
    float rho0;
    float m;
    float epsilon;
    float c;
    float dt;
    float max_relative_dp;
    float gx;
    float gy;
    float gz;
};


float W(float h, float3 p);
float3 gradW(float h, float3 p);

float W(float h, float3 p){
    float d = length(p);
    if(d<=h){
        float C = 315./(64.*3.1415926535*pow(h,3.f));
        float a = d/h;
        float b = 1-a*a;
        return C*pow(b,3.f);
    }
    return 0.f;
}

float3 gradW(float h, float3 p){
  float d = length(p);
  if(d<h){
      float C = -6.f*315.f/(64.f*3.1415926535*pow(h,5.f));
      float a = d/h;
      float b = 1-a*a;
      return C*pow(b,2.f)*p;
  }else{
    float3 zero = {0.f,0.f,0.f};
      return zero;
  }
}

// kernel called before the iterative solver, update v with the gravity, 
// and compute the nexte position for each particles, before the correction
__kernel void befor_solver(__global const struct sph_parameters* param, __global const float3 *p, __global float3 *v, __global float3 *q){
    int i = get_global_id(0);
    float3 g = {param->gx, param->gy, param->gz};
    v[i]  += param->dt * g;
    q[i] = p[i] + param->dt * v[i];
}

// Update the position, from the position given y the solver
// Compute the new speed
__kernel void update_position_speed(__global const struct sph_parameters* param, __global const float3 *q, __global float3 *p, __global float3 *v_copy){
    int i = get_global_id(0);
    v_copy[i] = (q[i] - p[i])/param->dt;
    p[i] = q[i];
}

// compute w for the calcul of the vorticity
__kernel void update_w(__global const struct sph_parameters* param, __global const float3 *p, __global const int *neighbors, __global const int *n_neighbors, __global const float3 *v_copy, __global float3 *w){
    int i = get_global_id(0);
    w[i] = float3(0.f, 0.f, 0.f);
    int n = min(param->nb_neighbors-1, n_neighbors[i]);
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[param->nb_neighbors * i + j_idx];
        w[i] += - param->m * cross(v_copy[j]-v_copy[i], gradW(param->h,p[i]-p[j]));
    }
}

// Apply the vorticity to each particles
__kernel void apply_vorticity(__global const struct sph_parameters* param, __global const float3 *p,  __global const int *neighbors, __global const int *n_neighbors, __global float3 *v_copy, __global const float3 *w){
    int i = get_global_id(0);
    int n = min(param->nb_neighbors-1, n_neighbors[i]);
    float3 eta = float3(0.f, 0.f, 0.f);
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[param->nb_neighbors * i + j_idx];
        eta += (length(w[j])-length(w[i]))/(length(p[j]-p[i])*length(p[j]-p[i]))*(p[j]-p[i]);
    }
    eta = normalize(eta);
    v_copy[i] += param->dt*param->h*0.001f*cross(eta,w[i]); //0.004 is maximum for stable
}

// apply the viscosity to each particles
__kernel void apply_viscosity(__global const struct sph_parameters* param, __global const float3 *p, __global const int *neighbors, __global const int *n_neighbors, __global const float3 *v_copy, __global float3 *v){
    int i = get_global_id(0);
    float alpha = 0;
    int n = min(param->nb_neighbors-1, n_neighbors[i]);
    float3 zero = {0.f,0.f,0.f};
    v[i] = zero;
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[param->nb_neighbors * i + j_idx];
        float dalpha = param->c * W(param->h, p[i] - p[j]) / W(param->h, float3(0,0,0));
        v[i] += dalpha* v_copy[j];
        alpha += dalpha;
    }
    //alpha = min(1.f,alpha);
    v[i] += (1-alpha) * v_copy[i];
}

// compute the pressure at each particle, for logging
__kernel void compute_pressure(__global const struct sph_parameters* param, __global const float3 *p, __global const int *neighbors, __global const int *n_neighbors, __global float *pressure){
    int i = get_global_id(0);
    int n = min(param->nb_neighbors-1, n_neighbors[i]);
    float rho = 0.f;
    for (int j_idx = 0; j_idx < n; j_idx++) {
        int j = neighbors[param->nb_neighbors * i + j_idx];
        rho += W(param->h, p[i] - p[j]);
    }
    rho *= param->m;
    pressure[i] = rho/param->rho0;
}
