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


uint hash(int x, int y, int z);


uint hash(int x, int y, int z) {
    return z*3884 + y*10+x;
}

// Fill the hashmap with each particle in the corresponding position
__kernel void fill_hashmap(__global const struct sph_parameters* param, __global const float3 *p, __global int *table, __global int *table_count) {
    int i = get_global_id(0);
    int x = floor(p[i].x/param->h);
    int y = floor(p[i].y/param->h);
    int z = floor(p[i].z/param->h);
    uint hash_xyz = hash(x, y, z);
    int idx = (hash_xyz % param->hash_table_size);
    int delta = atomic_inc(table_count  + idx);
    table[idx * param->table_list_size + min(delta, param->table_list_size-1)] = i;
}


// Look into the hashmap to find the potential neighbors of each particle  
__kernel void find_neighbors(__global const struct sph_parameters* param, __global const float3 *p, __global const int *table,  __global const int *table_count, __global int *neighbors, __global int *n_neighbors) {
    int i = get_global_id(0);
    int x = floor(p[i].x/param->h);
    int y = floor(p[i].y/param->h);
    int z = floor(p[i].z/param->h);
    int count = 0;
    #pragma unroll 3
    for(int dx = -1; dx<2;dx++) {
        #pragma unroll 3
        for(int dy = -1; dy<2;dy++) {
            #pragma unroll 3
            for(int dz = -1; dz<2;dz++) {
                int idx = hash(x+dx, y+dy, z+dz) % param->hash_table_size;
                int n = min(table_count[idx], param->table_list_size - 1);
                for (char d_idx = 0; d_idx < n; d_idx++) {
                    int j = table[idx*param->table_list_size + d_idx];
                    float3 dp = p[i] - p[j];
                    float dij2 = dp.x*dp.x + dp.y*dp.y + dp.z*dp.z;
                    if (dij2 < param->h*param->h && i!=j) {
                        neighbors[i*param->nb_neighbors + min(count, param->nb_neighbors-1)] =j;
                        count++;
                    }
                }
            }
        }
    }
    n_neighbors[i] = count;
}