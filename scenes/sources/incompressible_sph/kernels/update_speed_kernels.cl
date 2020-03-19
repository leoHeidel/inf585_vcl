__constant int nb_particles=16;
__constant int hash_table_size=36;
__constant int table_list_size=40;
__constant int nb_neighbors=40;
__constant float h = 0.1f;

__constant float rho0 = 1000.0f;
__constant float m = 1;
__constant float epsilon = 1e-3f;
__constant float c = 0.07;
__constant float dt = 0.02f;

float W(float3 p);

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
