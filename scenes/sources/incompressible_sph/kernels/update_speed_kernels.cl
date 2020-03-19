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

__kernel void befor_solver(__global const struct sph_parameters* param, __global const float3 *p, __global float3 *v, __global float3 *q){
    int i = get_global_id(0);
    float3 g = {0.f, -param->h * 50, 0.f};
    v[i]  += param->dt * g;
    q[i] = p[i] + param->dt * v[i];
}

__kernel void update_position_speed(__global const struct sph_parameters* param, __global const float3 *q, __global float3 *p, __global float3 *v_copy){
    int i = get_global_id(0);
    v_copy[i] = (q[i] - p[i])/param->dt;
    p[i] = q[i];
}

__kernel void apply_viscosity(__global const struct sph_parameters* param, __global const float3 *p, __global const int *neighbors, __global const int *n_neighbors, __global const float3 *v_copy, __global float3 *v){
    int i = get_global_id(0);
    float alpha = 0;
    int n = min(param->nb_neighbors, n_neighbors[i]);
    float3 zero = {0.f,0.f,0.f};
    v[i] = zero;
    for (int j_idx=0; j_idx < n; j_idx++) {
        int j = neighbors[param->nb_neighbors * i + j_idx];
        float dalpha = param->c * W(param->h, p[i] - p[j]) / W(param->h, float3(0,0,0));
        v[i] += dalpha* v_copy[j];
        alpha += dalpha;
    }
    alpha = min(1.f,alpha);
    v[i] += (1-alpha) * v_copy[i];
}
