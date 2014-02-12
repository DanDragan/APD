#include <fstream>
#include <mpi.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <queue>

using namespace std;

vector<int> neighbours;
int numtasks, rank;
bool isLeaf;
int **mat;
int *next_hop;
int parent;
int *votesp, *votesv;
int pres, vicepres;

// functia citeste din fisier doar linia corespunzatoare nodului
void readFile(char *filename) {

    ifstream input(filename);
    char *line = new char[256];
    for(int lineno = 0; input.getline(line, 256) && lineno < numtasks; lineno++) {
        if (lineno == rank) {
            string s(line);
            istringstream iss(s);
            string extra;
            iss >> extra;
            iss >> extra;
            while (iss) {
                int nr;
                if(iss >> nr) {
                    neighbours.push_back(nr);
                }
            }
            break;
        }
    }
    input.close();
}

// trimitere mesaje de broadcast si afisarea mesajului de final de
// de etapa dupa primirea a n-1 raspunsuri
void end_state(int nr) {
    char *line = new char[256];
    int stag = 6, stop_msg = 0;
    MPI_Status Stat;

    sprintf(line, "%d STOP", rank);

    for(int i = 0; i < numtasks; i++) {
        if(i != rank) {
            MPI_Send(line, 256, MPI_CHAR, i, stag, MPI_COMM_WORLD);
        }
    }
    
    while(stop_msg < numtasks - 1) {
        MPI_Recv(line, 256, MPI_CHAR, MPI_ANY_SOURCE, stag, MPI_COMM_WORLD, &Stat);
        stop_msg++;
    }
    printf("Sunt nodul %d si am primit mesaj de finalizare etapa %d de la restul nodurilor\n", rank, nr);
}

// determina matricea de adiacenta, informatie ce va fi cunoscuta de fiecare nod
void find_topology() {
    
    int sondtag = 1;
    int topotag = 2;
    parent = -1;
    MPI_Status Stat;
    int **tmp_mat = new int*[numtasks];
    mat = new int*[numtasks];
    for (int i = 0; i < numtasks; ++i) {
        mat[i] = new int[numtasks];
        tmp_mat[i] = new int[numtasks];
    }
    
    for(int i = 0; i < numtasks; i++) {
        for(int j = 0; j < numtasks; j++) {
            mat[i][j] = 0;
            tmp_mat[i][j] = 0;    
        }
    }
    
    for(unsigned int i = 0; i < neighbours.size(); i++) {
        mat[rank][neighbours[i]] = 1;
        mat[neighbours[i]][rank] = 1;
    }
        
    if(rank == 0) {
        for(unsigned int i = 0; i < neighbours.size(); i++) {
            MPI_Send(&rank, 1, MPI_INT, neighbours[i], sondtag, MPI_COMM_WORLD);
        }
    }
    else {
        int father;
        // primeste mesaj de sondaj de la parinte
        MPI_Recv(&father, 1, MPI_INT, MPI_ANY_SOURCE, sondtag, MPI_COMM_WORLD,
                &Stat);
       
        parent = father;
        
        if(neighbours.size() == 1) {
            isLeaf = true;
        }
        else {
            isLeaf = false;
        }
        // trimite tuturor copiilor (sau vecinilor pe legatura ce inchide un ciclu)
        for(unsigned int i = 0; i < neighbours.size(); i ++) {
            if(neighbours[i] != parent) {
                MPI_Send(&rank, 1, MPI_INT, neighbours[i], sondtag, MPI_COMM_WORLD);
            }
        }

        MPI_Request request[numtasks];
        int extra;
        int pass;
        
        // verificare primire extra mesaje de la alte noduri
        // in afara de parinte; se vor elimina legaturile
        for(unsigned int i = 0; i < neighbours.size(); i ++) {
            MPI_Irecv(&extra, 1, MPI_INT, neighbours[i], sondtag, MPI_COMM_WORLD, &request[neighbours[i]]);
        }
        
        sleep(1);

        for(unsigned int i = 0; i < neighbours.size(); i ++) {
            MPI_Test(&request[neighbours[i]], &pass, &Stat);

            if(pass) {
                for(unsigned int j = 0; j < neighbours.size(); j++) {
                    if(neighbours[j] == Stat.MPI_SOURCE) {
                        neighbours.erase(neighbours.begin() + j);
                     }
                }
                if(neighbours.size() == 1) {
                    isLeaf = true;
                }
                
                mat[rank][Stat.MPI_SOURCE] = 0;
                mat[Stat.MPI_SOURCE][rank] = 0;
            }
        }
    }
    // frunzele vor incepe trimiterea topologiei proprii
    if(isLeaf) {
        for(int i = 0; i < numtasks; i++) {
            MPI_Send(mat[i], numtasks, MPI_INT, parent, topotag, MPI_COMM_WORLD);
        }
    }
    // topologia se obtine adaugand informatiile proprii la cele primite
    if(!isLeaf && rank != 0) {
        for(unsigned int i = 0; i < neighbours.size(); i++) {
            if(neighbours[i] != parent) {
                for(int j = 0; j < numtasks; j++) {
                    MPI_Recv(tmp_mat[j], numtasks, MPI_INT, neighbours[i], topotag, MPI_COMM_WORLD, &Stat);
                }
                for(int x = 0; x < numtasks; x++) {
                    for(int y = 0; y < numtasks; y++) {
                        if(tmp_mat[x][y]) {
                            mat[x][y] = tmp_mat[x][y];
                        }
                    }
                }
            }
        }
        // se trimite topologia calculata la parinte
        for(int i = 0; i < numtasks; i++) {
            MPI_Send(mat[i], numtasks, MPI_INT, parent, topotag, MPI_COMM_WORLD);
        }
    }
    
    // radacina afiseaza topologia si o propaga inapoi pana la frunze
    if(rank == 0) {
        for(unsigned int i = 0; i < neighbours.size(); i++) {
            for(int j = 0; j < numtasks; j++) {
                MPI_Recv(tmp_mat[j], numtasks, MPI_INT, neighbours[i], topotag, MPI_COMM_WORLD, &Stat);
            }
            for(int x = 0; x < numtasks; x++) {
                for(int y = 0; y < numtasks; y++) {
                    if(tmp_mat[x][y]) {
                        mat[x][y] = tmp_mat[x][y];
                    }
                }
            }
        }
        printf("Topologia retelei este urmatoarea:\n");
        for(int i = 0; i < numtasks; i++) {
            for(int j = 0; j < numtasks; j++) {
                printf("%d ", mat[i][j]);
            }
            printf("\n");
        }
        printf("\n");
        
        for(unsigned int i = 0; i < neighbours.size(); i ++) {
            for(int j = 0; j < numtasks; j++) {
                MPI_Send(mat[j], numtasks, MPI_INT, neighbours[i], topotag, MPI_COMM_WORLD);
            }
        }
    }
    
    else {
        for(int i = 0; i < numtasks; i++) {
            MPI_Recv(mat[i], numtasks, MPI_INT, parent, topotag, MPI_COMM_WORLD, &Stat);
        }

        for(unsigned int i = 0; i < neighbours.size(); i++) {
            if(neighbours[i] != parent) {
                for(int j = 0; j < numtasks; j++) {
                    MPI_Send(mat[j], numtasks, MPI_INT, neighbours[i], topotag, MPI_COMM_WORLD);
                }
            }
        }
    }
}

// calculeaza hop-ul urmator pentru fiecare nod folosind BFS
void insert_next_hop(int *next_hop, bool *visited) {
    
    queue<int> q;
    q.push(rank);
    
    while(!q.empty()) {
        int new_node = q.front();
        visited[new_node] = true;
        q.pop();
        for(int i = 0; i < numtasks; i++) {
            if(mat[new_node][i] == 1 && !visited[i]) {
                next_hop[i] = next_hop[i] == -1 ? next_hop[new_node] : next_hop[i];
                q.push(i);
            }
        }
    }
}

// initializari variabile inainte de determinarea
// next-hopului pentru fiecare nod
int *get_routes() {

    int *next_hop = new int[numtasks];    
    bool *visited = new bool[numtasks];
    
    for(int i = 0; i < numtasks; i++)
        visited[i] = false;
    
    for(int i = 0; i < numtasks; i++) {
        next_hop[i] = mat[rank][i] != 0 ? i : -1;
    }
    next_hop[rank] = rank;
    
    insert_next_hop(next_hop, visited);
    
    return next_hop;        
}

// trimitere mesaje
void send_mail(char *filename) {

    ifstream input(filename);
    int no_msg;
    int tag = 3, stop_msg = 0;
    int source;
    char destination[10];
    char line[256];
    MPI_Status Stat;
    
    //citeste numarul de mesaje
    input.getline(line, 256);
    string s(line);
    istringstream iss(s);
    iss >> no_msg;
    
    for(int i = 0; i < no_msg; i++) {
        input.getline(line, 256);
        string s(line);
        istringstream iss(s);
        iss >> source;
        iss >> destination;
        if(source == rank) {
            // trimitere mesaj de broadcast
            if(strcmp(destination, "B") == 0) {
                for(unsigned int i = 0; i < neighbours.size(); i++) {
                    MPI_Send(line, 256, MPI_CHAR, neighbours[i], tag, MPI_COMM_WORLD);
                }
            }
            // trimitere mesaj normal
            else {
                MPI_Send(line, 256, MPI_CHAR, next_hop[atoi(destination)], tag, MPI_COMM_WORLD);
            }
        }
    }
    
    sprintf(line, "%d STOP", rank);
    // trimitere mesaje incheiere transmisie
    for(unsigned int i = 0; i < neighbours.size(); i++) {
        MPI_Send(line, 256, MPI_CHAR, neighbours[i], tag, MPI_COMM_WORLD);
    }
    // asteapta sa primeasca n-1 mesaje de broadcast
    while(stop_msg < numtasks - 1) {
        MPI_Recv(line, 256, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat);

        string s(line);
        istringstream iss(s);
        iss >> source;
        iss >> destination;
                
        if(strncmp(destination, "STOP", 4) != 0) {
             printf("Node: %d\nFrom: %d\nTo: %s\nMessage: %s\n\n", rank, source, destination, line);
           
            if(strcmp(destination, "B") == 0) {
                for(unsigned int i = 0; i < neighbours.size(); i++) {
                    if(neighbours[i] != Stat.MPI_SOURCE) {
                        MPI_Send(line, 256, MPI_CHAR, neighbours[i], tag, MPI_COMM_WORLD);
                    }
                }
            }
            // nod de legatura intre sursa si destinatia mesajului
            else if(rank != atoi(destination)) {
                MPI_Send(line, 256, MPI_CHAR, next_hop[atoi(destination)], tag, MPI_COMM_WORLD);
            }
        }
        else {
            stop_msg++;

            for(unsigned int i = 0; i < neighbours.size(); i++) {
                 if(neighbours[i] != Stat.MPI_SOURCE) {
                    MPI_Send(line, 256, MPI_CHAR, neighbours[i], tag, MPI_COMM_WORLD);
                }
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    printf("Sunt nodul %d si am primit mesaj de finalizare etapa 2 de la restul nodurilor\n", rank);
    input.close();
}

// alege un numar aleator
int choice() {
    srand(rank * (unsigned)time(0));
    int random_choice = rand() % numtasks;
    
    return random_choice; 
}

// calculeaza prima si a 2-a pozitia a celor mai mari valori dintr-un vector
void determine_leaders(int *v, int nr) {
    int max, smax, spos, pos;
    if(v[0] > v[1]) {
        max = v[0];
        pos = 0;
        smax = v[1];
        spos = 1;
    }
    else {
        max = v[1];
        pos = 1;
        smax = v[0];
        spos = 0;
    }

    for(int i = 2; i < nr; ++i) {
        if(v[i] > smax) {
            if(v[i] > max) {
                smax = max;
                spos = pos;
                max = v[i];
                pos = i;
            }
            else {
                smax = v[i];
                spos = i;
            }
        }
    }
    pres = pos;
    vicepres = spos;
}

// alegere lider
void choose_leaders() {
    
    pres = choice();
    vicepres = -1;

    int vicetag = 4;
    int prestag = 5;
    votesp = new int[numtasks];
    int *tmp_pres = new int[numtasks];
    MPI_Status Stat;
    
    for(int i = 0; i < numtasks; i++) {
        votesp[i] = 0;
    }    
    votesp[pres] += 1;
    
    // frunzele incep trimiterea alegerii
    if(isLeaf) {
        MPI_Send(votesp, numtasks, MPI_INT, parent, prestag, MPI_COMM_WORLD);
    }
    // celelalte noduri primesc voturile si le aduga si pe cele proprii, apoi retrimit
    if(!isLeaf && rank != 0) {
        
        for(unsigned int i = 0; i < neighbours.size(); i++) {
            if(neighbours[i] != parent) {
                MPI_Recv(tmp_pres, numtasks, MPI_INT, neighbours[i], prestag, MPI_COMM_WORLD, &Stat);
                
                for(int k = 0; k < numtasks; k++) {
                    votesp[k] += tmp_pres[k];
                }
            }
        }
        MPI_Send(votesp, numtasks, MPI_INT, parent, prestag, MPI_COMM_WORLD);
    }
    // radacina va centraliza voturile si va afla liderii, dupa care va
    // trimite informatiile si celorlalte noduri
    if(rank == 0) {
        for(unsigned int i = 0; i < neighbours.size(); i++) {
            MPI_Recv(tmp_pres, numtasks, MPI_INT, neighbours[i], prestag, MPI_COMM_WORLD, &Stat);
            
            for(int k = 0; k < numtasks; k++) {
                votesp[k] += tmp_pres[k];
            }
        }
        
        determine_leaders(votesp, numtasks);
                
        for(unsigned int i = 0; i < neighbours.size(); i ++) {
            MPI_Send(&pres, 1, MPI_INT, neighbours[i], prestag, MPI_COMM_WORLD);
            MPI_Send(&vicepres, 1, MPI_INT, neighbours[i], vicetag, MPI_COMM_WORLD);
        }
    }
    
    else {
        MPI_Recv(&pres, 1, MPI_INT, parent, prestag, MPI_COMM_WORLD, &Stat);
        MPI_Recv(&vicepres, 1, MPI_INT, parent, vicetag, MPI_COMM_WORLD, &Stat);

        for(unsigned int i = 0; i < neighbours.size(); i++) {
            if(neighbours[i] != parent) {
                MPI_Send(&pres, 1, MPI_INT, neighbours[i], prestag, MPI_COMM_WORLD);
                MPI_Send(&vicepres, 1, MPI_INT, neighbours[i], vicetag, MPI_COMM_WORLD);
            }
        }
    }
    printf("\n%d %d %d\n", rank, pres, vicepres);
}

int main(int argc, char* argv[]) {
          
    MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    readFile(argv[1]);
    find_topology();
    next_hop = get_routes();
    
    char *mesaj = new char[100];
    sprintf(mesaj, "Sunt nodul %d si am tabela de rutare:\n", rank);
    string msg(mesaj);
    for(int i = 0; i < numtasks; i++) {
        if(i != rank) {
            sprintf(mesaj, "%d %d\n", i, next_hop[i]);
            msg += string(mesaj);
        }
    }
    msg += string("\n");
    printf("%s", msg.c_str());
    
    MPI_Barrier(MPI_COMM_WORLD);
    end_state(1);
    MPI_Barrier(MPI_COMM_WORLD);
    
    send_mail(argv[2]);
    MPI_Barrier(MPI_COMM_WORLD);
    choose_leaders();
    
    MPI_Finalize();
    return 0;
}
