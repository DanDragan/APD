//333CA Dragan Dan-Stefan

#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdlib.h>


int type;
double x_min, y_min, x_max, y_max;
double resolution;
int max_steps;
double xc, yc;
int width, height;
int num_colors = 256;
int chunkSize, offstart, offend;
int rank, size;
int *image, *offsets;

//citeste datele din fisierul de intrare
void parse_file(char *filename) {

    FILE *input = fopen(filename, "r");
    fscanf(input, "%d", &type);
    fscanf(input, "%lf %lf %lf %lf", &x_min, &x_max, &y_min, &y_max);
    fscanf(input, "%lf", &resolution);
    fscanf(input, "%d", &max_steps);
    
    if(type == 1) {
        fscanf(input, "%lf %lf", &xc, &yc);
    }
    
    fclose(input);
}

//determina dimensiunile imaginii si aloca memorie pentru ea
void init_image() {
    
    double iwidth, iheight;
    width = (x_max - x_min) / resolution;
    height = (y_max - y_min) / resolution;
    modf(width, &iwidth);
    modf(height, &iheight);
    width = (int)iwidth;
    height = (int)iheight;
    
    image  = malloc(sizeof(int) * width * height);
    offsets = malloc(sizeof(int) * (size + 1));
}

//aplica unul din cei 2 algoritmi
void mandelbrot_julia() {
    int j = 0;
    double cx, cy;
    double zx, zy;
    
    for(cy = y_min + offstart * resolution; cy < y_max && j < offend - offstart; cy += resolution, ++j) {
        int i = 0;
        for(cx = x_min; cx < x_max && i < width; cx += resolution, ++i) {
            int step = 0;
            if(type == 0) {
                zx = 0;
                zy = 0;
            }
            else {
                zx = cx;
                zy = cy;
            }
            
            while(sqrt(zx * zx + zy * zy) < 2 && step < max_steps) {
                double tzx = zx;
                if(type == 0) {
                    zx = zx * zx - zy * zy + cx;
                    zy = 2 * tzx * zy + cy;
                }
                else {
                    zx = zx * zx - zy * zy + xc;
                    zy = 2 * tzx * zy + yc;
                }
                step++;
             }   
             int color = step % num_colors;
             image[j * width + i] = color;
            
        }
    }
}

//masterul imparte fiecarui task spatiul din imagine pe care lucreaza
void distribute_offsets(int size) {
    int i, tag = 1;
    
    if(rank == 0) {
        chunkSize = height / size;

        int end;
        
        int offset = 0;
        offstart = 0;
        offend = chunkSize;
        offsets[0] = 0;
        offsets[1] = offend;
        for(i = 1; i < size; i++) {
            offset += chunkSize;
            if(i != size - 1) {
                end = offset + chunkSize;
                offsets[i+1] = end;
            }
            else {
                end = height;
                offsets[i+1] = end;    
            }
            MPI_Send(&offset, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
            MPI_Send(&end, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        }        
    }
    else {
        MPI_Status Stat;
        MPI_Recv(&offstart, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&offend, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
        
        image = malloc(sizeof(int) * width * (offend - offstart));
    }
}

//taskurile ii trimit masterului bucatile din imagine
void collect_chunks() {
    
    int tag = 1;
    int i;
    MPI_Status Stat;
    
    if(rank != 0) {
        MPI_Send(image, width * (offend - offstart), MPI_INT, 0, tag, MPI_COMM_WORLD);
    }
    else {
        for(i = 1; i < size; i ++) {
            if(i != size - 1) {
                MPI_Recv(&image[offsets[i] * width], chunkSize *
                    width, MPI_INT, i, tag, MPI_COMM_WORLD, &Stat);
            }
            else {
                MPI_Recv(&image[offsets[i] * width], (chunkSize + (height % size)) *
                    width, MPI_INT, i, tag, MPI_COMM_WORLD, &Stat);
            }
        }
    }
}

//se scrie in fisierul PGM datele
void write_image(char *image_name) {
    
    int i, j;
    FILE *output = fopen(image_name, "w");
    
    fprintf(output, "P2\n");
    fprintf(output, "%d %d\n", width, height);
    fprintf(output, "%d\n", num_colors - 1);
    
    for(i = height - 1; i >= 0; i--) {
        for(j = 0; j < width; ++j) {
            fprintf(output, "%d ", image[i * width + j]);
        }
        fprintf(output, "\n");
    }
    fclose(output);
}

int main(int argc, char *argv[]) {
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int i, tag = 1;
    
    if(rank == 0) {
        parse_file(argv[1]);
        init_image();
        
        //se trimit informatiile citite din fisier
        for(i = 1; i < size; i++) {
            MPI_Send(&x_min, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&x_max, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&y_min, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&y_max, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&resolution, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&xc, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&yc, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
            MPI_Send(&max_steps, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
            MPI_Send(&type, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
            MPI_Send(&width, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
            MPI_Send(&height, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        }
    }
    //taskurile primesc informatiile citite din fisier si calculate de master
    else {
        MPI_Status Stat;
        
        MPI_Recv(&x_min, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&x_max, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&y_min, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&y_max, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&resolution, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&xc, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&yc, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&max_steps, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&type, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&width, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&height, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
    }
    
    distribute_offsets(size);
    
    mandelbrot_julia();
    
    collect_chunks();
    
    if(rank == 0) {
        write_image(argv[2]);
    }
    
    free(image);
    
    MPI_Finalize();
    
    return 0;
}
