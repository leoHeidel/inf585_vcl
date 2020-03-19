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
};


float W(float h, float3 p);
float3 gradW(float h, float3 p);

float W(float h, float3 p){
    float d = length(p);
    if(d<=h){
        float C = 315./(64.*3.1415926535*pow(h,3.));
        float a = d/h;
        float b = 1-a*a;
        return C*pow(b,3.);
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


<<<<<<< HEAD
__kernel void compute_constraints(__global const struct sph_parameters* param, __global const float3 *q, __global const int *neighbors, 
=======
__kernel void compute_constraints(__global const float3 *q, __global const int *neighbors,
>>>>>>> 1607dc22ab7da0e02baea750cd73883765db5b64
      __global const int *n_neighbors, __global float *lambda) {
  int i = get_global_id(0);
  int n = min(param->nb_neighbors, n_neighbors[i]);
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
<<<<<<< HEAD
  rho *= param->m; 
  lambda[i] = - (rho - param->rho0) * param->rho0 / (sum + param->epsilon);
  lambda[i] /= param->m * param->m; 
}

__kernel void compute_dp(__global const struct sph_parameters* param, __global const float3 *q, __global const int *neighbors, 
=======
  rho *= m;
  lambda[i] = - (rho - rho0) * rho0 / (sum + epsilon);
  lambda[i] /= m * m;
}

__kernel void compute_dp(__global const float3 *q, __global const int *neighbors,
>>>>>>> 1607dc22ab7da0e02baea750cd73883765db5b64
      __global const int *n_neighbors, __global const float *lambda, __global float3 *dp){
  int i = get_global_id(0);
  int n = min(param->nb_neighbors, n_neighbors[i]);
  float3 zero = {0.f,0.f,0.f};
  dp[i] = zero;
  for (int j_idx = 0; j_idx < n; j_idx++) {
    int j = neighbors[param->nb_neighbors * i + j_idx];
    float s = 0; //- 0.1f * pow(W(particles[i].q - particles[j].q)/W(vcl::vec3(0.2f*sph_param.h, 0.f, 0.f)), 4.f); // homogeneous h^-3
    dp[i] += (lambda[i] + lambda[j] + s) * gradW(param->h, q[i] - q[j]); // homogeneous h^-2;
  }
  dp[i] *= param->m / param->rho0;
  float coef = 0.1;
  float d = length(dp[i]);
<<<<<<< HEAD
  d = d < param->h * coef ? 1 : d / (param->h * coef) ;
  dp[i] /= d; 
=======
  d = d < h * coef ? 1 : d / (h * coef) ;
  dp[i] /= d;
>>>>>>> 1607dc22ab7da0e02baea750cd73883765db5b64
}

__kernel void solve_collisions(__global const struct sph_parameters* param,  __global const float3 *q, __global float3 *dp){
    int i = get_global_id(0);
    float3 d = q[i]+ dp[i];
    float eps = 0.01f;
    d.x = clamp(d.x, -1.f + eps*(i / (float) param->nb_particles), 1.f - eps*(i / (float) param->nb_particles));
    d.y = clamp(d.y, -1.f + eps*(i / (float) param->nb_particles), 1.f - eps*(i / (float) param->nb_particles));
    d.z = clamp(d.z, -1.f + eps*(i / (float) param->nb_particles), 1.f - eps*(i / (float) param->nb_particles));
    dp[i] =  d - q[i];
}

__kernel void add_position_correction(__global const float3 *dp, __global float3 *q){
  int i = get_global_id(0);
  q[i] += dp[i];
}
