#ifndef ZEIGEN_H
#define ZEIGEN_H

#include <ostream>

class zCMgcEigenD {
public:

    zCMgcEigenD(int size);
    ~zCMgcEigenD();

    void Tridiagonal2(double** matrix, double* diagonal, double* subdiagonal);
    void Tridiagonal3(double** matrix, double* diagonal, double* subdiagonal);
    void Tridiagonal4(double** matrix, double* diagonal, double* subdiagonal);
    void TridiagonalN(int size, double** matrix, double* diagonal, double* subdiagonal);
    void QLAlgorithm(int size, double* diagonal, double* subdiagonal, double** matrix);
    void DecreasingSort(int size, double* eigenvalues, double** eigenvectors);
    void IncreasingSort(int size, double* eigenvalues, double** eigenvectors);
    zCMgcEigenD& Matrix(double** matrix);
    void EigenStuff2();
    void EigenStuff3();
    void EigenStuff4();
    void EigenStuffN();
    void EigenStuff();
    void DecrSortEigenStuff2();
    void DecrSortEigenStuff3();
    void DecrSortEigenStuff4();
    void DecrSortEigenStuffN();
    void DecrSortEigenStuff();
    void IncrSortEigenStuff2();
    void IncrSortEigenStuff3();
    void IncrSortEigenStuff4();
    void IncrSortEigenStuffN();
    void IncrSortEigenStuff();
    static int Number(unsigned int error);
    static void Report(unsigned int error);
    static void Report(std::ostream& stream);

    int size;
    double** mat;
    double* diag;
    double* subd;

    static int verbose;
    static unsigned int error;
    static const unsigned int invalid_size;
    static const unsigned int allocation_failed;
    static const unsigned int ql_exceeded;
    static const char* message[5];
};

#endif
