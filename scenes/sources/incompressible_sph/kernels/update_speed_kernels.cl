__constant int nb_particles=2000;
__constant int hash_table_size=36;
__constant int table_list_size=40;
__constant int nb_neighbors=40;
__constant float h = 0.1f;

__constant float rho0 = 1000.0f;
__constant float m = 1;
__constant float epsilon = 1e-3f;
__constant float c = 0.1;
__constant float dt = 0.02f;
__constant float eps = 10.f;

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


__kernel void befor_solver(__global const float3 *p, __global float3 *v, __global float3 *q){
    int i = get_global_id(0);
    float3 g = {0.f, -h * 50, 0.f};
    v[i]  += dt * g;
    q[i] = p[i] + dt * v[i];
}

__kernel void update_position_speed(__global const float3 *q, __global float3 *p, __global float3 *v_copy){
    int i = get_global_id(0);
    v_copy[i] = (q[i] - p[i])/dt;
    p[i] = q[i];
}

__kernel void update_w(__global const float3 *p, __global const int *neighbors, __global const int *n_neighbors, __global const float3 *v_copy, __global float3 *w){
    int i = get_global_id(0);
    w[i] = float3(0.f, 0.f, 0.f);
    int n = min(nb_neighbors, n_neighbors[i]);
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[nb_neighbors * i + j_idx];
        w[i] += - m * cross(v_copy[j]-v_copy[i], gradW(p[i]-p[j]));
    }
}

__kernel void apply_vorticity(__global const float3 *p,  __global const int *neighbors, __global const int *n_neighbors, __global float3 *v_copy, __global const float3 *w){
    int i = get_global_id(0);
    int n = min(nb_neighbors, n_neighbors[i]);
    float3 eta = float3(0.f, 0.f, 0.f);
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[nb_neighbors * i + j_idx];
        eta += (length(w[j])-length(w[i]))/(length(p[j]-p[i])*length(p[j]-p[i]))*(p[j]-p[i]);
    }
    eta = normalize(eta);
    v_copy[i] += dt*eps*cross(eta,w[i]);
}

__kernel void apply_viscosity(__global const float3 *p, __global const int *neighbors, __global const int *n_neighbors, __global const float3 *v_copy, __global float3 *v){
    int i = get_global_id(0);
    float alpha = 0;
    int n = min(nb_neighbors, n_neighbors[i]);
    float3 zero = {0.f,0.f,0.f};
    v[i] = zero;
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[nb_neighbors * i + j_idx];
        float dalpha = c * W(p[i] - p[j]) / W(float3(0,0,0));
        v[i] += dalpha* v_copy[j];
        alpha += dalpha;
    }
    alpha = min(1.f,alpha);
    v[i] += (1-alpha) * v_copy[i];
}
