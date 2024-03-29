/* 
    compilazione:
        g++ merge.cpp ordinamento.cpp -I../include
    esecuzione:
        ./a.out 20 -g && dot graph.dot -Tpdf -o graph.pdf && open graph.pdf
        
    Obiettivo: 
        disegnare la sequenza di ordinamenti effettuata e la struttura delle chiamate ricorsive
        --> 1 Creazione nodi e numerazione univoca per tracciare la ricorsione
        --> 2 Preparazione disegno con Graphviz dot
        --> 3 Nodo che descrive il sotto-array corrente (in verde conto l'ordine in cui e' chiamata l'attività)

        esecuzioni (per osservare struttura ricorsiva)
        ./a.out 15 -graph; dot graph.dot -Tpdf -o graph.pdf
        ./a.out 16 -graph; dot graph.dot -Tpdf -o graph.pdf
        ./a.out 17 -graph; dot graph.dot -Tpdf -o graph.pdf

        prove con array ordinato crescente, decrescente o random 
        --> come cambia la struttura ricorsiva? 
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/ordinamento.hh"

using namespace std;

Stat stat; // Per le statistiche
int& ct_swap = stat.ct_swap;
int& ct_cmp = stat.ct_cmp;
int& ct_read = stat.ct_read;
int& n = stat.n;
int& max_dim = stat.max_dim;
int& ntests = stat.ntests;
int& ndiv = stat.ndiv;
bool& details = stat.details;
bool& graph = stat.graph;
bool& comparison = stat.comparison;
string& output_path = stat.output_path;
ofstream& output_graph = stat.output_graph;
int global_count_recursion = 0;

void print_array_graph(int *A, int a, int b, string c) {
    /// prepara il disegno dell'array A ed il suo contenuto dall'indice a all'indice b inclusi

    /// scommentare per creare un nodo con dettagli

    output_graph << c << a << "_" << b << " [label=<" << endl
                 << "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">" << endl;
    output_graph << "<TR><TD bgcolor=\"#80f0b0\">" << global_count_recursion++ << "</TD></TR><TR>";
    for (int j = a; j <= b; j++) {
        output_graph << "<TD>" << A[j] << "</TD>" << endl;
    }
    output_graph << "</TR> </TABLE>>];" << endl;
}

void merge(int *A, int p, int q, int r, int *L, int *R) {

    /// copia valori delle due meta p..q e q+1..r
    int i = 0;
    int j = 0;
    int k = 0;

    for (i = 0; i < q - p + 1; i++)
        L[i] = A[p + i];
    L[i] = 1000000; /// un numero grande

    for (i = 0; i < r - q; i++)
        R[i] = A[q + 1 + i];
    R[i] = 1000000; /// un numero grande

    //// dettagli
    if (details) {
        cout << "Array L (" << p << " .. " << q << "): ";
        print_array(L, q - p + 1);
        cout << "Array R (" << q + 1 << " .. " << r << "): ";
        print_array(R, r - q);
    }

    i = 0; /// usato per gestire array L
    j = 0; /// usato per gestire array R

    for (k = p; k <= r; k++) {
        ct_cmp++;
        if (L[i] <= R[j]) {
            A[k] = L[i];
            i++;
        } else {
            A[k] = R[j];
            j++;
        }
    }
}

void merge_sort(int *A, int p, int r, int *L, int *R) {
    /// gli array L e R sono utilizzati come appoggio per copiare i valori: evita le allocazioni nella fase di merge
    if (p < r) {
        int q = (p + r) / 2;

        if (graph) {
            print_array_graph(A, p, q, "node_before");
            print_array_graph(A, q + 1, r, "node_before");
            /// disegna arco p_r -> p_q
            output_graph << "node_before" << p << "_" << r << " -> ";
            output_graph << "node_before" << p << "_" << q << endl;
            /// disegna arco p_r -> q+1_r
            output_graph << "node_before" << p << "_" << r << " -> ";
            output_graph << "node_before" << q + 1 << "_" << r << endl;
        }

        merge_sort(A, p, q, L, R);
        merge_sort(A, q + 1, r, L, R);

        if (graph) {
            print_array_graph(A, p, q, "node_after");
            print_array_graph(A, q + 1, r, "node_after");
            /// disegna arco p_q -> p_r
            output_graph << "node_after" << p << "_" << q << " -> ";
            output_graph << "node_after" << p << "_" << r << endl;
            /// disegna arco q+1_r -> p_r
            output_graph << "node_after" << q + 1 << "_" << r << " -> ";
            output_graph << "node_after" << p << "_" << r << endl;
        }

        merge(A, p, q, r, L, R);
        if (details)
            print_array(A, n);

    } else {
        /// disegno archi nodi singoli
        output_graph << "node_before" << p << "_" << r << " -> ";
        output_graph << "node_after" << p << "_" << r << endl;
    }
}

int main(int argc, char **argv) {
    int i, test;
    int *A;
    int *L; /// usato come buffer di appoggio
    int *R; /// usato come buffer di appoggio

    if (parse_cmd(argc, argv, stat))
        return 1;

    /// allocazione array
    A = new int[max_dim];
    L = new int[max_dim + 1];
    R = new int[max_dim + 1];

    // init random
    srand((unsigned)time(NULL));

    if (graph) {
        output_graph.open(output_path);
        
        if (!output_graph.is_open()) {
            cerr << "Impossibile aprire il file di output" << endl;
            cerr << "Path: " << output_path << endl;
            return 1;
        }
        /// preparo header
        output_graph << "digraph g" << endl;
        output_graph << "{ " << endl;
        output_graph << "node [shape=plaintext]" << endl;
    }

    if (ndiv > 1)
        cout << "Dim_array,N_test,min_swap,avg_swap,max_swap,worst_case_swap,min_cmp,avg_cmp,max_cmp,worst_case_cmp\n";

    //// inizio ciclo per calcolare 100 dimensioni di array crescenti
    for (n = max_dim / ndiv; n <= max_dim; n += max_dim / ndiv) {
        int swap_min = -1;
        int swap_max = -1;
        long swap_avg = 0;

        int cmp_min = -1.0;
        int cmp_max = -1.0;
        long cmp_avg = 0;

        //// lancio ntests volte per coprire diversi casi di input random
        for (test = 0; test < ntests; test++) {
            /// inizializzazione array: numeri random con range dimensione array
            for (i = 0; i < n; i++) {
                // if (graph)
                // A[i]= n - i;
                //
                // else
                A[i] = rand() % (3 * n);
                // A[i]= i;
            }

            if (details) {
                cout << "creato array di dimensione " << n << "\n";
                print_array(A, n);
            }

            ct_swap = 0;
            ct_cmp = 0;

            /// algoritmo di sorting
            if (graph)
                print_array_graph(A, 0, n - 1, "node_before");

            merge_sort(A, 0, n - 1, L, R);

            if (graph)
                print_array_graph(A, 0, n - 1, "node_after");

            /// statistiche
            swap_avg += ct_swap;
            if (swap_min < 0 || swap_min > ct_swap)
                swap_min = ct_swap;
            if (swap_max < 0 || swap_max < ct_swap)
                swap_max = ct_swap;
            cmp_avg += ct_cmp;
            if (cmp_min < 0 || cmp_min > ct_cmp)
                cmp_min = ct_cmp;
            if (cmp_max < 0 || cmp_max < ct_cmp)
                cmp_max = ct_cmp;
        }

        // cout << (long)(n*(long)(log(n)/log(2)) + 2*( n-pow(2,(long)(log(n)/log(2))) )) << endl;

        if (ndiv > 1)
            cout << n << "," << ntests << "," << swap_min << "," << (0.0 + swap_avg) / ntests << "," << swap_max << "," << 0.0 << "," << cmp_min << "," << (0.0 + cmp_avg) / ntests << "," << cmp_max << "," << n * log(n) / log(2) << "\n";
    }

    if (graph) {
        /// preparo footer e chiudo file
        output_graph << "}" << endl;
        output_graph.close();
        cout << "File " << output_path << " scritto" << endl
             << "Creare il grafo con: dot " << output_path << " -Tpdf -o graph.pdf" << endl;
    }

    delete[] A;
    delete[] L;
    delete[] R;

    return 0;
}
