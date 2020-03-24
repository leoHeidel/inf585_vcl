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

// Compute the constrain: lamda for each particles
__kernel void compute_constraints(__global const struct sph_parameters* param, __global const float3 *q, __global const int *neighbors,
      __global const int *n_neighbors, __global float *lambda) {
  int i = get_global_id(0);
  int n = min(param->nb_neighbors-1, n_neighbors[i]);
  float rho = 0.f;
  float3 ci= {0.f,0.f,0.f};
  float sum = 0.f;
  for (int j_idx = 0; j_idx < n; j_idx++) {
    int j = neighbors[param->nb_neighbors * i + j_idx];
    rho += W(param->h, q[i] - q[j]);
    float3 grad_ij = gradW(param->h, q[i] - q[j]);
    ci += grad_ij;
    sum += dot(grad_ij,grad_ij);
  }
  sum += dot(ci,ci);
  rho *= param->m;
  lambda[i] = - (rho - param->rho0) * param->rho0 / (sum + param->epsilon);
  lambda[i] /= param->m * param->m;
}

// From the constraints, compute the nex dp
__kernel void compute_dp(__global const struct sph_parameters* param, __global const float3 *q, __global const int *neighbors,
      __global const int *n_neighbors, __global const float *lambda, __global float3 *dp){
  int i = get_global_id(0);
  int n = min(param->nb_neighbors-1, n_neighbors[i]);
  float3 zero = {0.f,0.f,0.f};
  dp[i] = zero;
  for (int j_idx = 0; j_idx < n; j_idx++) {
    int j = neighbors[param->nb_neighbors * i + j_idx];
    float3 dq = {0.1f*param->h, 0.f, 0.f};
    float s = - 0.1f * pow(W(param->h, q[i] - q[j])/W(param->h, dq), 4.f); // homogeneous h^-3
    dp[i] += (lambda[i] + lambda[j] + s) * gradW(param->h, q[i] - q[j]); // homogeneous h^-2;
  }
  dp[i] *= param->m / param->rho0;
  float d = length(dp[i]);
  d = d < param->h * param->max_relative_dp ? 1 : d / (param->h * param->max_relative_dp) ;
  dp[i] /= d;
}

//Enforce that the particles stay confined in the box
__kernel void solve_collisions(__global const struct sph_parameters* param,  __global const float3 *q, __global float3 *dp){
    int i = get_global_id(0);
    float3 d = q[i]+ dp[i];
    float eps = 0.01f;
    d.x = clamp(d.x, -1.f + 0.3f*((float) param->h) + eps*(i / (float) param->nb_particles), 1.f - 0.3f*((float) param->h) - eps*(i / (float) param->nb_particles));
    d.y = clamp(d.y, -1.f + 0.3f*((float) param->h) + eps*(i / (float) param->nb_particles), 1.f - 0.3f*((float) param->h) - eps*(i / (float) param->nb_particles));
    d.z = clamp(d.z, -1.f + 0.3f*((float) param->h) + eps*(i / (float) param->nb_particles), 1.f - 0.3f*((float) param->h) - eps*(i / (float) param->nb_particles));
    dp[i] =  d - q[i];
}

// apply the result of the solver step
__kernel void add_position_correction(__global const float3 *dp, __global float3 *q){
  int i = get_global_id(0);
  q[i] += dp[i];
}
