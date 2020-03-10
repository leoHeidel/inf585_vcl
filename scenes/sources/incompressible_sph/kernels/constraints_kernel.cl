__kernel void compute_constraints(__global const float3 q, __global const int *neighbors, __global float *lambda) {

}


void scene_model::compute_constraints(){
  for(auto& particle : particles){
    particle.rho = 0.f;
    vcl::vec3 ci = vec3(0.f, 0.f, 0.f);
    float sum = 0.f;
    for(size_t j : particle.neighbors){
      particle.rho += W(particle.q - particles[j].q); // homogeneous h^-3
      ci += gradW(particle.q - particles[j].q); // homogeneous h^-4
      float grad_j_norm = norm(gradW(particle.q - particles[j].q)); //homogeneous h^-4
      sum += grad_j_norm*grad_j_norm;
      if (sph_param.verbose && &particle == &(particles[0])) std::cout << "added to rho/m : " <<  W(particle.q - particles[j].q) << std::endl;
    }
    float n = norm(ci); // homogeneous h^-4
    sum += n * n; // homogeneous h^-8
    particle.rho *= sph_param.m; // homogeneous h^0
    particle.lambda = - (particle.rho - sph_param.rho0) * sph_param.rho0 / (sum + sph_param.eps);// homogeneous h^8
    particle.lambda /= sph_param.m * sph_param.m; // homogeneous h^2
  }
  if (sph_param.verbose) std::cout << "rho : " << particles[sph_param.verbose_index].rho << " rho0 : " << sph_param.rho0 << std::endl;
  if (sph_param.verbose) std::cout << "m : " << sph_param.m << " max w : " << W({0.f,0.f,0.f}) << std::endl;
  if (sph_param.verbose) std::cout << "lambda : " << particles[sph_param.verbose_index].lambda << std::endl;
}
