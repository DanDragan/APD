// 333CA Dragan Dan-Stefan

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


int main(int argc, char **argv) {
    
    FILE *input = fopen(argv[2], "r");
    FILE *output = fopen(argv[3], "w");
    int N, parties, i, j, k, x;
    int no_weeks = atoi(argv[1]), week, lim;
    fscanf(input, "%d %d", &N, &parties);
    
    // forum, newforum - matricile de senatori
    // senators, newsenators - vectori cu numarul de senatori de o culoare
    // colors - distanta minima pentru fiecare culoare
    int forum[N][N], colors[parties], newforum[N][N];
    int senators[N], newsenators[N];
        
    for(i = 0; i < N; i++)
        for(j = 0; j < N; j++)
            fscanf(input, "%d", &forum[i][j]);
    
          
    for(i = 0; i < parties; i++)
        newsenators[i] = N;
    

    for(x = 0; x < parties; x++) {
        colors[x] = N;
    }
    
    for(week = 0; week < no_weeks; week++) {
        for(x = 0; x < parties; x++) {
            senators[x] = 0;
        }
        
        // pentru fiecare element din matrice se verifica elementele din jurul
        // acestuia, aflate la o distanta incrementala, pornind de la 1,
        // pentru a vedea daca distanta de la el catre celalalt
        // element e mai mica decat distanta minima a culorii
        
        // impartire pe threaduri a scanarii fiecarui element din matrice 
        #pragma omp parallel for private(j, k, lim, x, colors)                 
        for(i = 0; i < N ; i++) {
            for(j = 0; j < N; j++) {
                int crtparties = 0;
                for(x = 0; x < parties; x++) {
                    if(newsenators[x] > 0) {
                        colors[x] = N;
                        crtparties++;
                    }
                    else
                        colors[x] = 0;                    
                }

                if(newsenators[forum[i][j]] == 1) {
                    colors[forum[i][j]] = 0;
                    crtparties--;   
                }
                
                int radius = 1;
                int count = 0;
                
                // numarul de partide a caror distanta a fost modificata e mai
                // mic decat numarul de partide ramase de la pasul anterior
                while(count < crtparties) {
                    
                    // elementele de deasupra
                    if(i-radius >= 0 && i-radius < N) {
                        k = j >= radius ? -radius : -j;
                        lim = radius <= N - j - 1 ? radius : N - j - 1;                     
                        for(; k <= lim; k++) {                            
                            if(radius < colors[forum[i-radius][j+k]]) {
                                colors[forum[i-radius][j+k]] = radius;
                                count++;
                            }
                        }
                    }
                    // elementele de dedesubt    
                    if(i+radius >= 0 && i+radius < N) {
                        k = j >= radius ? -radius : -j;
                        lim = radius <= N - j - 1 ? radius : N - j - 1;                                
                        for(; k <= lim ; k++) { 
                            if(radius < colors[forum[i+radius][j+k]]) {
                                colors[forum[i+radius][j+k]] = radius;
                                count++;
                            }
                        }
                    }   
                    
                    // elementele din stanga
                    if(j-radius >= 0 && j-radius < N) {
                        k = i >= radius + 1 ? -radius : -i;
                        lim = radius < N - i - 1 ? radius : N - i - 1;    
                        for(; k <= lim; k++) {
                            if(radius < colors[forum[i+k][j-radius]]) {
                                colors[forum[i+k][j-radius]] = radius;
                                count++;
                            }
                        }
                    }
                    // elementele din dreapta    
                    if(j+radius >= 0 && j+radius < N) {
                        k = i >= radius + 1 ? -radius : -i;
                        lim = radius < N - i - 1 ? radius : N - i - 1;
                        for(; k <= lim; k++) {
                            if(radius < colors[forum[i+k][j+radius]]) {
                                colors[forum[i+k][j+radius]] = radius;
                                count++;
                            }
                        }
                    }             
                                                            
                    radius++;
                }
                
                    int maxim = colors[0];
                    int val = 0;
                    for(x = 1; x < parties; x++) {
                        if(maxim < colors[x]) {
                            maxim = colors[x];
                            val = x;                        
                        }                
                    }
                    newforum[i][j] = val;
                // incrementarea numarului de senatori de o anumita
                // culoare se face intr-o zona critica
                #pragma omp critical
                {
                    senators[val]++;
                }        
            }
        }
        

        for(i = 0; i < parties; i++)
            newsenators[i] = senators[i];

        for(i = 0; i < parties; i++)
            fprintf(output, "%d ", newsenators[i]);
        fprintf(output, "\n");

        for(i = 0; i < N; i++)
            for(j = 0; j < N; j++)
                forum[i][j] = newforum[i][j];
    }
        
    for(i = 0; i < N; i++) {
        for(j = 0; j < N; j++)
            fprintf(output, "%d ", newforum[i][j]);
        fprintf(output, "\n");
    }
    fclose(input);   
    fclose(output);
            
    return 0;
}
