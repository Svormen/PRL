/*
 * Projekt: PRL - Parallel Splitting
 * Meno: Slavomir Svorada (xsvora02)
 * Datum: 23.3.2023
 */

#include "mpi.h"
#include <stdio.h>
#include <vector>
#include <fstream>

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
        cout << myValues[i] << " ";
    }
    cout << endl;
}

int main(int argc, char *argv[])
{
    int rank, size;
    int median;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // printf("I am %d of %d\n", rank, size);

    vector<int> scatter;
    // vector for saving number from file
    vector<int> myValues;
    int mySize;

    // vectors for less, equal and qreater values
    vector<int> lVal;
    vector<int> eVal;
    vector<int> gVal;

    // zero processor reads data and share data to others
    if (rank == 0)
    {
        // call function for getting numbers from file
        readNumbers(&myValues);
        //printf("Data: ");
        //showValues(myValues);

        // find median
        if (myValues.size() % 2 == 0)
        {
            median = myValues[(myValues.size() / 2) - 1];
        }
        else
        {
            median = myValues[myValues.size() / 2];
        }
        //cout << "Median is: " << median << endl;

        mySize = (myValues.size() / size);
    }

    // sending the median to other processes using MPI_Bcast
    MPI_Bcast(&median, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mySize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    scatter.resize(mySize);
    // use MPI_Scatter for divide numbers between processes
    MPI_Scatter(myValues.data(), mySize, MPI_INT, scatter.data(), mySize, MPI_INT, 0, MPI_COMM_WORLD);

    // divide number into 3 vectors
    for (int i = 0; i < scatter.size(); i++)
    {
        if (scatter[i] < median)
        {
            lVal.push_back(scatter[i]);
        }
        else if (scatter[i] > median)
        {
            gVal.push_back(scatter[i]);
        }
        else
        {
            eVal.push_back(scatter[i]);
        }
    }

    // length of vectors (less, equal, greater)
    int vectorL = (int)lVal.size();
    int vectorE = (int)eVal.size();
    int vectorG = (int)gVal.size();

    // we will gather the vector sizes to the root process, where we will create a buffer for receiving the concatenated data
    const int myRoot = 0;
    int *recvSum[] = {NULL, NULL, NULL};
    vector<int> concateData[3];

    // only root get received data
    if (rank == myRoot)
    {
        for (int i = 0; i < 3; i++)
        {
            recvSum[i] = (int *)malloc(size * sizeof(int));
            concateData[i].reserve(size);
        }
    }

    MPI_Gather(&vectorL, 1, MPI_INT, recvSum[0], 1, MPI_INT, myRoot, MPI_COMM_WORLD);
    MPI_Gather(&vectorE, 1, MPI_INT, recvSum[1], 1, MPI_INT, myRoot, MPI_COMM_WORLD);
    MPI_Gather(&vectorG, 1, MPI_INT, recvSum[2], 1, MPI_INT, myRoot, MPI_COMM_WORLD);

    // compute the aggregate length of the string, and determine the individual displacements for each rank
    int finalLen[] = {0, 0, 0};
    int *shows[] = {NULL, NULL, NULL};

    if (rank == myRoot)
    {
        for (int i = 0; i < 3; i++)
        {
            shows[i] = (int *)malloc(size * sizeof(int));
            shows[i][0] = 0;
            finalLen[i] += recvSum[i][0];

            for (int j = 1; j < size; j++)
            {
                finalLen[i] += recvSum[i][j];
                shows[i][j] = shows[i][j - 1] + recvSum[i][j - 1];
            }
            concateData[i].resize(finalLen[i]);
        }
    }

    // now that we have initialized the receive buffer, counts, and displacements, we can proceed with gathering the strings
    MPI_Gatherv(lVal.data(), vectorL, MPI_INT, concateData[0].data(), recvSum[0], shows[0], MPI_INT, myRoot, MPI_COMM_WORLD);
    MPI_Gatherv(eVal.data(), vectorE, MPI_INT, concateData[1].data(), recvSum[1], shows[1], MPI_INT, myRoot, MPI_COMM_WORLD);
    MPI_Gatherv(gVal.data(), vectorG, MPI_INT, concateData[2].data(), recvSum[2], shows[2], MPI_INT, myRoot, MPI_COMM_WORLD);

    if (rank == myRoot)
    {
        // print 
        const char *names[] = {"L: ", "E: ", "G: "};
        for (int i = 0; i < 3; i++)
        {
            cout << names[i];
            showValues(concateData[i]);
        }
    }

    MPI_Finalize();
    return 0;
}
