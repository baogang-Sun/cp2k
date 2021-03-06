/*****************************************************************************
 *  CP2K: A general program to perform molecular dynamics simulations        *
 *  Copyright (C) 2000 - 2020  CP2K developers group                         *
 *****************************************************************************/

#define _XOPEN_SOURCE 700   /* Enable POSIX 2008/13 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <stdlib.h>
#include <stdarg.h>

#include "grid_collocate_replay.h"
#include "grid_collocate_cpu.h"

// *****************************************************************************
void grid_collocate_record(const bool orthorhombic,
                           const bool use_subpatch,
                           const int subpatch,
                           const int border,
                           const int func,
                           const int la_max,
                           const int la_min,
                           const int lb_max,
                           const int lb_min,
                           const double zeta,
                           const double zetb,
                           const double rscale,
                           const double dh[3][3],
                           const double dh_inv[3][3],
                           const double ra[3],
                           const double rab[3],
                           const int npts_global[3],
                           const int npts_local[3],
                           const int shift_local[3],
                           const double radius,
                           const int o1,
                           const int o2,
                           const int n1,
                           const int n2,
                           const double pab[n2][n1],
                           const double* grid){

    static int counter = 0;
    counter++;
    char filename[100];
    snprintf(filename, sizeof(filename), "grid_collocate_%05i.task", counter);

    const int D = DECIMAL_DIG;  // In C11 we could use DBL_DECIMAL_DIG.
    FILE *fp = fopen(filename, "w+");
    fprintf(fp, "#Grid collocate task v8\n");
    fprintf(fp, "orthorhombic %i\n", orthorhombic);
    fprintf(fp, "use_subpatch %i\n", use_subpatch);
    fprintf(fp, "subpatch %i\n", subpatch);
    fprintf(fp, "border %i\n", border);
    fprintf(fp, "func %i\n", func);
    fprintf(fp, "la_max %i\n", la_max);
    fprintf(fp, "la_min %i\n", la_min);
    fprintf(fp, "lb_max %i\n", lb_max);
    fprintf(fp, "lb_min %i\n", lb_min);
    fprintf(fp, "zeta %.*e\n", D, zeta);
    fprintf(fp, "zetb %.*e\n", D, zetb);
    fprintf(fp, "rscale %.*e\n", D, rscale);
    for (int i=0; i<3; i++)
        fprintf(fp, "dh %i %.*e %.*e %.*e\n", i, D, dh[i][0], D, dh[i][1], D, dh[i][2]);
    for (int i=0; i<3; i++)
        fprintf(fp, "dh_inv %i %.*e %.*e %.*e\n", i, D, dh_inv[i][0], D, dh_inv[i][1], D, dh_inv[i][2]);
    fprintf(fp, "ra %.*e %.*e %.*e\n", D, ra[0], D, ra[1], D, ra[2]);
    fprintf(fp, "rab %.*e %.*e %.*e\n", D, rab[0], D, rab[1], D, rab[2]);
    fprintf(fp, "npts_global %i  %i %i\n", npts_global[0], npts_global[1], npts_global[2]);
    fprintf(fp, "npts_local %i  %i %i\n", npts_local[0], npts_local[1], npts_local[2]);
    fprintf(fp, "shift_local %i  %i %i\n", shift_local[0], shift_local[1], shift_local[2]);
    fprintf(fp, "radius %.*e\n", D, radius);
    fprintf(fp, "o1 %i\n", o1);
    fprintf(fp, "o2 %i\n", o2);
    fprintf(fp, "n1 %i\n", n1);
    fprintf(fp, "n2 %i\n", n2);

    for (int i=0; i < n2; i++) {
    for (int j=0; j < n1; j++) {
        fprintf(fp, "pab %i %i %.*e\n", i, j, D, pab[i][j]);
    }
    }

    const int npts_local_total = npts_local[0] * npts_local[1] * npts_local[2];

    int ngrid_nonzero = 0;
    for (int i=0; i<npts_local_total; i++) {
        if (grid[i] != 0.0) {
            ngrid_nonzero++;
        }
    }
    fprintf(fp, "ngrid_nonzero %i\n", ngrid_nonzero);

    for (int k=0; k<npts_local[2]; k++) {
    for (int j=0; j<npts_local[1]; j++) {
    for (int i=0; i<npts_local[0]; i++) {
        const double val =  grid[k*npts_local[1]*npts_local[0] + j*npts_local[0] + i];
        if (val != 0.0) {
            fprintf(fp, "grid %i %i %i %.*e\n", i, j, k, D, val);
        }
    }
    }
    }
    fprintf(fp, "#THE_END\n");
    fclose(fp);
    printf("Wrote %s\n", filename);

}

// *****************************************************************************
static void read_next_line(char line[], int length, FILE *fp) {
    if (fgets(line, length, fp) == NULL) {
        fprintf(stderr, "Error: Could not read line.\n");
        abort();
    }
}

// *****************************************************************************
static void parse_next_line(const char key[], FILE *fp, const char format[],
                            const int nargs, ...) {
    char line[100];
    read_next_line(line, sizeof(line), fp);

    char full_format[100];
    strcpy(full_format, key);
    strcat(full_format, " ");
    strcat(full_format, format);

    va_list varargs;
    va_start(varargs, nargs);
    if (vsscanf(line, full_format, varargs) != nargs) {
        fprintf(stderr, "Error: Could not parse line.\n");
        fprintf(stderr, "Line: %s\n", line);
        fprintf(stderr, "Format: %s\n", full_format);
        abort();
    }
}

// *****************************************************************************
int parse_int(const char key[], FILE *fp) {
    int value;
    parse_next_line(key, fp, "%i", 1, &value);
    return value;
}

// *****************************************************************************
void parse_int3(const char key[], FILE *fp, int vec[3]) {
    parse_next_line(key, fp, "%i %i %i", 3, &vec[0], &vec[1], &vec[2]);
}

// *****************************************************************************
double parse_double(const char key[], FILE *fp) {
    double value;
    parse_next_line(key, fp, "%le", 1, &value);
    return value;
}

// *****************************************************************************
void parse_double3(const char key[], FILE *fp, double vec[3]) {
    parse_next_line(key, fp, "%le %le %le", 3, &vec[0], &vec[1], &vec[2]);
}

// *****************************************************************************
void parse_double3x3(const char key[], FILE *fp, double mat[3][3]) {
    char format[100];
    for (int i=0; i<3; i++) {
        sprintf(format, "%i %%le %%le %%le", i);
        parse_next_line(key, fp, format, 3, &mat[i][0], &mat[i][1], &mat[i][2]);
    }
}

// *****************************************************************************
double grid_collocate_replay(const char* filename, const int cycles){
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Could not open task file: %s\n", filename);
        exit(1);
    }

    char header_line[100];
    read_next_line(header_line, sizeof(header_line), fp);
    if (strcmp(header_line, "#Grid collocate task v8\n") != 0) {
        fprintf(stderr, "Error: Wrong file header.\n");
        abort();
    }

    const bool orthorhombic = parse_int("orthorhombic", fp);
    const bool use_subpatch = parse_int("use_subpatch", fp);
    const int subpatch = parse_int("subpatch", fp);
    const int border = parse_int("border", fp);
    const int func = parse_int("func", fp);
    const int la_max = parse_int("la_max", fp);
    const int la_min = parse_int("la_min", fp);
    const int lb_max = parse_int("lb_max", fp);
    const int lb_min = parse_int("lb_min", fp);
    const double zeta = parse_double("zeta", fp);
    const double zetb = parse_double("zetb", fp);
    const double rscale = parse_double("rscale", fp);

    double dh[3][3], dh_inv[3][3], ra[3], rab[3];
    parse_double3x3("dh", fp, dh);
    parse_double3x3("dh_inv", fp, dh_inv);
    parse_double3("ra", fp, ra);
    parse_double3("rab", fp, rab);

    int npts_global[3], npts_local[3], shift_local[3];
    parse_int3("npts_global", fp, npts_global);
    parse_int3("npts_local", fp, npts_local);
    parse_int3("shift_local", fp, shift_local);

    const double radius = parse_double("radius", fp);
    const int o1 = parse_int("o1", fp);
    const int o2 = parse_int("o2", fp);
    const int n1 = parse_int("n1", fp);
    const int n2 = parse_int("n2", fp);

    double pab[n2][n1];
    char format[100];
    for (int i=0; i<n2; i++) {
    for (int j=0; j<n1; j++) {
        sprintf(format, "%i %i %%le", i, j);
        parse_next_line("pab", fp, format, 1, &pab[i][j]);
    }
    }

    const int npts_local_total = npts_local[0] * npts_local[1] * npts_local[2];
    const size_t sizeof_grid = sizeof(double) * npts_local_total;
    double* grid_ref = malloc(sizeof_grid);
    memset(grid_ref, 0, sizeof_grid);

    const int ngrid_nonzero = parse_int("ngrid_nonzero", fp);
    for (int n=0; n < ngrid_nonzero; n++) {
        int i, j, k;
        double value;
        parse_next_line("grid", fp, "%i %i %i %le", 4, &i, &j, &k, &value);
        grid_ref[k*npts_local[1]*npts_local[0] + j*npts_local[0] + i] = value;
    }

    char footer_line[100];
    read_next_line(footer_line, sizeof(footer_line), fp);
    if (strcmp(footer_line, "#THE_END\n") != 0) {
        fprintf(stderr, "Error: Wrong footer line.\n");
        abort();
    }

    double* grid_test = malloc(sizeof_grid);
    memset(grid_test, 0, sizeof_grid);

    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    for (int i=0; i < cycles ; i++) {
        grid_collocate_pgf_product_cpu(orthorhombic,
                                       use_subpatch,
                                       subpatch,
                                       border,
                                       func,
                                       la_max,
                                       la_min,
                                       lb_max,
                                       lb_min,
                                       zeta,
                                       zetb,
                                       rscale,
                                       dh,
                                       dh_inv,
                                       ra,
                                       rab,
                                       npts_global,
                                       npts_local,
                                       shift_local,
                                       radius,
                                       o1,
                                       o2,
                                       n1,
                                       n2,
                                       pab,
                                       grid_test);
    }

    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    const double delta_sec = (end_time.tv_sec - start_time.tv_sec) + 1e-9 * (end_time.tv_nsec - start_time.tv_nsec);

    double max_value = 0.0;
    double max_diff = 0.0;
    for (int i=0; i < npts_local_total; i++) {
        const double ref_value = cycles * grid_ref[i];
        const double diff = fabs(grid_test[i] - ref_value);
        max_diff = fmax(max_diff, diff);
        max_value = fmax(max_value, fabs(grid_test[i]));
    }

    printf("Task: %-65s   Cycles: %e   Max value: %le   Max diff: %le   Time: %le sec\n",
           filename, (float)cycles, max_value, max_diff, delta_sec);

    free(grid_ref);
    free(grid_test);

    return max_diff;
}

//EOF
