//333CA Dragan Dan-Stefan

#include <stdio.h>
#include <stdlib.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))

int main(int argc, char **argv) {
    
    FILE *input = fopen(argv[2], "r");
    FILE *output = fopen(argv[3], "w");
    int N, parties, i, j, k, l, x;
    int no_weeks = atoi(argv[1]), week;
    fscanf(input, "%d %d", &N, &parties);
    
    // forum, newforum - matricile de senatori
    // senators, newsenators - vectori cu numarul de senatori de o culoare
    // colors - distanta minima pentru fiecare culoare
    int forum[N][N], colors[parties], newforum[N][N];
    int senators[parties], newsenators[parties];
        
    for(i = 0; i < N; i++)
        for(j = 0; j < N; j++)
            fscanf(input, "%d", &forum[i][j]);
           
    for(i = 0; i < parties; i++)
        newsenators[i] = N;
    
    // iteratii pentru fiecare saptamana
    for(week = 0; week < no_weeks; week++) {
        for(x = 0; x < parties; x++) {
            senators[x] = 0;
        }
        // pentru fiecare element din matrice se verifica toate celelalte
        // elemente pentru a vedea daca distanta de la el catre celalalt
        // element e mai mica decat distanta minima a culorii         
        for(i = 0; i < N; i++) {
            for(j = 0; j < N; j++) {
                
                for(x = 0; x < parties; x++) {
                    if(newsenators[x] > 0)
                        colors[x] = N;
                    else
                        colors[x] = 0;                    
                }
                // daca e singurul senator de culoarea respectiva
                // distanta culorii devine 0
                if(newsenators[forum[i][j]] == 1)
                    colors[forum[i][j]] = 0;
                
                for(k = 0; k < N; k++) {
                    for(l = 0; l < N; l++) {
                        if((i != k || j != l) && 
                        max(abs(i - k), abs(j - l)) < colors[forum[k][l]]) {
                            colors[forum[k][l]] = max(abs(i - k), abs(j - l));
                        }
                    }
                }
                
                // calculeaza maximul distantelor
                int maxim = colors[0];
                int val = 0;
                for(x = 1; x < parties; x++) {
                    if(maxim < colors[x]) {
                        maxim = colors[x];
                        val = x;                        
                    }                
                }
                newforum[i][j] = val;
                senators[val]++;
                          
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
