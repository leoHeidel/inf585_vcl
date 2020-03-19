__constant int nb_particles=16;
__constant int hash_table_size=36;
__constant int table_list_size=40;
__constant int nb_neighbors=40;
__constant float h = 0.1f;

__constant float rho0 = 1000.0f;
__constant float m = 1;
__constant float epsilon = 1e-3f;
__constant float c = 0.05;

float W(float3 p);
float3 gradW(float3 p);

float W(float3 p){
    float d = length(p);
    if(d<=h){
        float C = 315./(64.*3.1415926535*pow(h,3.));
        float a = d/h;
        float b = 1-a*a;
        return C*pow(b,3.);
    }
    return 0.f;
}

float3 gradW(float3 p){
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


__kernel void compute_constraints(__global const float3 *q, __global const int *neighbors, 
      __global const int *n_neighbors, __global float *lambda) {
  int i = get_global_id(0);
  int n = min(nb_neighbors, n_neighbors[i]);
  float rho = 0.f;
  float3 ci= {0.f,0.f,0.f};
  float sum = 0.f;
  for (int j_idx = 0; j_idx < n; j_idx++) {
    int j = neighbors[nb_neighbors * i + j_idx];
    rho += W(q[i] - q[j]);
    float3 grad_ij = gradW(q[i] - q[j]);
    ci += grad_ij;
    sum += dot(grad_ij,grad_ij);
  }
  sum += dot(ci,ci);
  rho *= m; 
  lambda[i] = - (rho - rho0) * rho0 / (sum + epsilon);
  lambda[i] /= m * m; 
}

__kernel void compute_dp(__global const float3 *q, __global const int *neighbors, 
      __global const int *n_neighbors, __global const float *lambda, __global float3 *dp){
  int i = get_global_id(0);
  int n = min(nb_neighbors, n_neighbors[i]);
  float3 zero = {0.f,0.f,0.f};
  dp[i] = zero;
  for (int j_idx = 0; j_idx < n; j_idx++) {
    int j = neighbors[nb_neighbors * i + j_idx];
    float s = 0; //- 0.1f * pow(W(particles[i].q - particles[j].q)/W(vcl::vec3(0.2f*sph_param.h, 0.f, 0.f)), 4.f); // homogeneous h^-3
    dp[i] += (lambda[i] + lambda[j] + s) * gradW(q[i] - q[j]); // homogeneous h^-2;
  }
  dp[i] *= m / rho0;
  float coef = 0.1;
  float d = length(dp[i]);
  d = d < h * coef ? 1 : d / (h * coef) ;
  dp[i] /= d; 
}

__kernel void solve_collisions(__global const float3 *q, __global float3 *dp){
    int i = get_global_id(0);
    float3 d = q[i]+ dp[i]; 
    float eps = 0.01f;
    d.x = clamp(d.x, -1.f + eps*(i / (float) nb_particles), 1.f - eps*(i / (float) nb_particles));
    d.y = clamp(d.y, -1.f + eps*(i / (float) nb_particles), 1.f - eps*(i / (float) nb_particles));
    d.z = clamp(d.z, -1.f + eps*(i / (float) nb_particles), 1.f - eps*(i / (float) nb_particles));
    dp[i] =  d - q[i];
}

__kernel void add_position_correction(__global const float3 *dp, __global float3 *q){
  int i = get_global_id(0);
  q[i] += dp[i];
}
