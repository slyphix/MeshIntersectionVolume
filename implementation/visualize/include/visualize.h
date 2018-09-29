#ifndef VISUALIZE_H
#define VISUALIZE_H

namespace visualize {

void init(int &argc, char **argv);

// terms: num_terms mal p t u n (12 float) sequentiell
// meshes: num_meshes oft len_mesh mal dreieck a b c (insgesamt 9 float) sequentiell
// colors: num_meshes oft c (3 float) sequentiell
int launch(int num_terms, float *terms_buffer, int num_meshes, int *len_meshes, float *mesh_buffer, float *colors = nullptr);

}

#endif
