#ifndef LLOYDMAX_CODEBOOK_H
#define LLOYDMAX_CODEBOOK_H

typedef struct {
    float centroids[256];
    float boundaries[257];
    float f_min;
    float f_max;
    int n_levels;
    float mse_uniform;
    float mse_lloydmax;
} LloydMaxCodebook;

#ifdef __cplusplus
extern "C" {
#endif

LloydMaxCodebook lloydmax_generate(int grid_size, int sim_steps);
void lloydmax_print_codebook_c(const LloydMaxCodebook* cb);

#ifdef __cplusplus
}
#endif

#endif
