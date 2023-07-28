/*
 * Projekt: PRL - K-MEANS algoritmus
 * Meno: Slavomir Svorada (xsvora02)
 * Datum: 13.4.2023
 */

#include "mpi.h"
#include <stdio.h>
#include <vector>
#include <fstream>
#include <climits>
#include <float.h>
#include <iomanip>
#include <cstdlib>

using namespace std;

// function for reading numbers from file
bool readNumbers(vector<int> *myValues)
{
    ifstream myFile;
    myFile.open("numbers");

    if (myFile.is_open())
    {
        int myInt;
        while (myFile.good())
        {
            myInt = myFile.get();
            if (!myFile.good())
            {
                break;
            }
            myValues->push_back(myInt);
        }
    }
    else
    {
        cerr << "Error during opening file." << endl;
        return 1;
    }
    return 0;
}

// function for showing data with space between each other
void showValues(vector<int> myValues)
{
    for (int i = 0; i < myValues.size(); i++)
    {
        if (i != (myValues.size() - 1))
        {
            cout << myValues[i] << ", ";
        }
        else
        {
            cout << myValues[i];
        }
    }
    cout << endl;
}

// function for finding nearest center (return index of center)
int closeCenter(int myVal, vector<float> centers)
{
    float findMin = DBL_MAX;
    int getIndex = 0;
    float distance;
    for (int i = 0; i < centers.size(); i++)
    {
        distance = abs(centers[i] - myVal);
        if (findMin > distance)
        {
            findMin = distance;
            getIndex = i;
        }
    }
    return getIndex;
}

// main function
int main(int argc, char *argv[])
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // vectors
    vector<float> centers;
    centers.resize(4);
    vector<float> centersNew;
    centersNew.resize(4);
    int scatter;
    // vector for saving number from file
    vector<int> myValues;
    int mySize;
    int procData;
    float final = 0;

    vector<int> processA;
    processA.resize(4);
    fill(processA.begin(), processA.end(), 0);
    vector<int> processB;
    processB.resize(4);
    fill(processB.begin(), processB.end(), 0);
    vector<int> procRes;
    procRes.resize(4);
    fill(procRes.begin(), procRes.end(), 0);
    vector<int> procRes2;
    procRes2.resize(4);
    fill(procRes2.begin(), procRes2.end(), 0);

    // vectors for divide number between 4 vectors depends on index
    vector<int> numCen0;
    vector<int> numCen1;
    vector<int> numCen2;
    vector<int> numCen3;

    // zero processor reads data and share data to others
    if (rank == 0)
    {
        // call function for getting numbers from file
        readNumbers(&myValues);
        mySize = myValues.size();
        // check count of numbers
        if (mySize > 32)
        {
            fprintf(stderr, "ERROR: zlý počet čísel. (>32)\n");
            exit(EXIT_FAILURE);
        }
        if (mySize < 4)
        {
            fprintf(stderr, "ERROR: zlý počet čísel. (<4)\n");
            exit(EXIT_FAILURE);
        }
        // save first 4 centers
        for (int i = 0; i < 4; i++)
        {
            centers[i] = (myValues[i]);
        }
    }

    // repeat until find final centers
    while(true)
    {
        // share centers to other proccesses
        MPI_Bcast(centers.data(), 4, MPI_FLOAT, 0, MPI_COMM_WORLD);
        
        // use MPI_Scatter for divide numbers between processes
        MPI_Scatter(myValues.data(), 1, MPI_INT, &scatter, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // set vector due to center
        procData = closeCenter(scatter, centers);
        //printf("PROCDATA: %d\n", procData);
        
        // save number and 1 to vectors
        processA[procData] = scatter;
        processB[procData] = 1;

        // use MPI_Reduce for sum data
        MPI_Reduce(processA.data(), procRes.data(), 4, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(processB.data(), procRes2.data(), 4, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Bcast(procRes.data(), 4, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(procRes2.data(), 4, MPI_INT, 0, MPI_COMM_WORLD);

        // print the result on rank 0
        if (rank == 0) {
            // get new centees
            for (int i = 0; i < 4; i++)
            {
                centersNew[i] = (float(procRes[i]) / float(procRes2[i]));
            }

            // we got the same centers
            if (centers == centersNew)
            {
                final = 1;
                // divide every number into vectors depends on distance
                for (int i = 0; i < size; i++)
                {
                    // find index for divide number between vectors
                    procData = closeCenter(myValues[i], centers);

                    if (procData == 0) {
                        numCen0.push_back(myValues[i]);
                    } else if (procData == 1) {
                        numCen1.push_back(myValues[i]);
                    } else if (procData == 2) {
                        numCen2.push_back(myValues[i]);
                    } else {
                        numCen3.push_back(myValues[i]);
                    }
                }
                
                // final output
                cout << "[" << centersNew[0] << "] ";
                showValues(numCen0);
                cout << "[" << centersNew[1] << "] ";
                showValues(numCen1);
                cout << "[" << centersNew[2] << "] ";
                showValues(numCen2);
                cout << "[" << centersNew[3] << "] ";
                showValues(numCen3);
            }

            // save new centers into the old centers
            for (int i = 0; i < 4; i++)
            {
                centers[i] = centersNew[i];
            }

        }
        
        MPI_Bcast(&final, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
        MPI_Bcast(centersNew.data(), 4, MPI_FLOAT, 0, MPI_COMM_WORLD);

        // we find same centers so do break
        if (final == 1) {
            break;
        }
    }

    MPI_Finalize();
    return 0;
}
