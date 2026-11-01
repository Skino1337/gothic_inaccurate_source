#include "include/zEigen.h"
#include <cmath>
#include <cstdio>

zCMgcEigenD::zCMgcEigenD(int sizeValue)
    : size(sizeValue)
{
    if (size <= 1) {
        Report(invalid_size);
        return;
    }
    mat = new double*[size];
    if (!mat) {
        Report(allocation_failed);
        return;
    }
    for (int i = 0; i < size; ++i) {
        mat[i] = new double[size];
        if (!mat[i]) {
            Report(allocation_failed);
            return;
        }
    }
    diag = new double[size];
    if (!diag) {
        Report(allocation_failed);
        return;
    }
    subd = new double[size];
    if (!subd)
        Report(allocation_failed);
}

zCMgcEigenD::~zCMgcEigenD()
{
    delete[] subd;
    delete[] diag;
    for (int i = 0; i < size; ++i)
        delete[] mat[i];
    delete[] mat;
}

void zCMgcEigenD::TridiagonalN(int dimension, double** matrix, double* diagonal,
    double* subdiagonal)
{
    for (int i = dimension - 1; i >= 1; --i) {
        const int last = i - 1;
        double scale = 0.0;
        double householder = 0.0;
        if (last > 0) {
            for (int k = 0; k <= last; ++k)
                scale += fabs(matrix[i][k]);
            if (scale == 0.0)
                subdiagonal[i] = matrix[i][last];
            else {
                for (int k = 0; k <= last; ++k) {
                    matrix[i][k] /= scale;
                    householder += matrix[i][k] * matrix[i][k];
                }
                double value = matrix[i][last];
                double root = sqrt(householder);
                if (value >= 0.0)
                    root = -root;
                subdiagonal[i] = scale * root;
                householder -= value * root;
                matrix[i][last] = value - root;
                value = 0.0;
                for (int j = 0; j <= last; ++j) {
                    matrix[j][i] = matrix[i][j] / householder;
                    double sum = 0.0;
                    for (int k = 0; k <= j; ++k)
                        sum += matrix[j][k] * matrix[i][k];
                    for (int k = j + 1; k <= last; ++k)
                        sum += matrix[k][j] * matrix[i][k];
                    subdiagonal[j] = sum / householder;
                    value += subdiagonal[j] * matrix[i][j];
                }
                const double half = value / (householder + householder);
                for (int j = 0; j <= last; ++j) {
                    const double rowValue = matrix[i][j];
                    subdiagonal[j] -= half * rowValue;
                    const double columnValue = subdiagonal[j];
                    for (int k = 0; k <= j; ++k)
                        matrix[j][k] -= rowValue * subdiagonal[k]
                            + columnValue * matrix[i][k];
                }
            }
        }
        else
            subdiagonal[i] = matrix[i][last];
        diagonal[i] = householder;
    }

    diagonal[0] = 0.0;
    subdiagonal[0] = 0.0;
    for (int i = 0; i < dimension; ++i) {
        const int last = i - 1;
        if (diagonal[i] != 0.0) {
            for (int j = 0; j <= last; ++j) {
                double sum = 0.0;
                for (int k = 0; k <= last; ++k)
                    sum += matrix[i][k] * matrix[k][j];
                for (int k = 0; k <= last; ++k)
                    matrix[k][j] -= sum * matrix[k][i];
            }
        }
        diagonal[i] = matrix[i][i];
        matrix[i][i] = 1.0;
        for (int j = 0; j <= last; ++j) {
            matrix[j][i] = 0.0;
            matrix[i][j] = 0.0;
        }
    }
    for (int i = 1; i < dimension; ++i)
        subdiagonal[i - 1] = subdiagonal[i];
    subdiagonal[dimension - 1] = 0.0;
}

void zCMgcEigenD::Tridiagonal2(double** matrix, double* diagonal, double* subdiagonal)
{
    TridiagonalN(2, matrix, diagonal, subdiagonal);
}

void zCMgcEigenD::Tridiagonal3(double** matrix, double* diagonal, double* subdiagonal)
{
    TridiagonalN(3, matrix, diagonal, subdiagonal);
}

void zCMgcEigenD::Tridiagonal4(double** matrix, double* diagonal, double* subdiagonal)
{
    TridiagonalN(4, matrix, diagonal, subdiagonal);
}

void zCMgcEigenD::QLAlgorithm(int dimension, double* diagonal, double* subdiagonal,
    double** matrix)
{
    for (int i = 0; i < dimension; ++i) {
        int iteration = 0;
        while (iteration < 30) {
            int pivot = i;
            while (pivot < dimension - 1) {
                const double sum = fabs(diagonal[pivot]) + fabs(diagonal[pivot + 1]);
                if (fabs(subdiagonal[pivot]) + sum == sum)
                    break;
                ++pivot;
            }
            if (pivot == i)
                break;

            const double delta = (diagonal[i + 1] - diagonal[i])
                / (2.0 * subdiagonal[i]);
            double root = sqrt(delta * delta + 1.0);
            root = delta < 0.0 ? delta - root : delta + root;
            double shift = diagonal[pivot] - diagonal[i] + subdiagonal[i] / root;
            double sine = 1.0;
            double cosine = 1.0;
            double previous = 0.0;
            for (int j = pivot - 1; j >= i; --j) {
                const double scaledSine = sine * subdiagonal[j];
                const double scaledCosine = cosine * subdiagonal[j];
                if (fabs(scaledSine) >= fabs(shift)) {
                    cosine = shift / scaledSine;
                    root = sqrt(cosine * cosine + 1.0);
                    subdiagonal[j + 1] = scaledSine * root;
                    sine = 1.0 / root;
                    cosine *= sine;
                }
                else {
                    sine = scaledSine / shift;
                    root = sqrt(sine * sine + 1.0);
                    subdiagonal[j + 1] = shift * root;
                    cosine = 1.0 / root;
                    sine *= cosine;
                }
                const double diagonalValue = diagonal[j + 1] - previous;
                const double rotation = (diagonal[j] - diagonalValue) * sine
                    + 2.0 * scaledCosine * cosine;
                previous = sine * rotation;
                diagonal[j + 1] = diagonalValue + previous;
                shift = cosine * rotation - scaledCosine;
                for (int k = 0; k < dimension; ++k) {
                    const double value = matrix[k][j + 1];
                    matrix[k][j + 1] = sine * matrix[k][j] + cosine * value;
                    matrix[k][j] = cosine * matrix[k][j] - sine * value;
                }
            }
            diagonal[i] -= previous;
            subdiagonal[i] = shift;
            subdiagonal[pivot] = 0.0;
            ++iteration;
        }
        if (iteration == 30) {
            Report(ql_exceeded);
            break;
        }
    }
}

void zCMgcEigenD::DecreasingSort(int dimension, double* eigenvalues, double** eigenvectors)
{
    for (int i = 0; i < dimension - 1; ++i) {
        int selected = i;
        for (int j = i + 1; j < dimension; ++j) {
            if (eigenvalues[j] > eigenvalues[selected])
                selected = j;
        }
        if (selected != i) {
            const double value = eigenvalues[selected];
            eigenvalues[selected] = eigenvalues[i];
            eigenvalues[i] = value;
            for (int j = 0; j < dimension; ++j) {
                const double vectorValue = eigenvectors[j][i];
                eigenvectors[j][i] = eigenvectors[j][selected];
                eigenvectors[j][selected] = vectorValue;
            }
        }
    }
}

void zCMgcEigenD::IncreasingSort(int dimension, double* eigenvalues, double** eigenvectors)
{
    for (int i = 0; i < dimension - 1; ++i) {
        int selected = i;
        for (int j = i + 1; j < dimension; ++j) {
            if (eigenvalues[j] < eigenvalues[selected])
                selected = j;
        }
        if (selected != i) {
            const double value = eigenvalues[selected];
            eigenvalues[selected] = eigenvalues[i];
            eigenvalues[i] = value;
            for (int j = 0; j < dimension; ++j) {
                const double vectorValue = eigenvectors[j][i];
                eigenvectors[j][i] = eigenvectors[j][selected];
                eigenvectors[j][selected] = vectorValue;
            }
        }
    }
}

zCMgcEigenD& zCMgcEigenD::Matrix(double** matrix)
{
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j)
            mat[i][j] = matrix[i][j];
    }
    return *this;
}

void zCMgcEigenD::EigenStuff2()
{
    Tridiagonal2(mat, diag, subd);
    QLAlgorithm(size, diag, subd, mat);
}

void zCMgcEigenD::EigenStuff3()
{
    Tridiagonal3(mat, diag, subd);
    QLAlgorithm(size, diag, subd, mat);
}

void zCMgcEigenD::EigenStuff4()
{
    Tridiagonal4(mat, diag, subd);
    QLAlgorithm(size, diag, subd, mat);
}

void zCMgcEigenD::EigenStuffN()
{
    TridiagonalN(size, mat, diag, subd);
    QLAlgorithm(size, diag, subd, mat);
}

void zCMgcEigenD::EigenStuff()
{
    if (size == 2)
        EigenStuff2();
    else if (size == 3)
        EigenStuff3();
    else if (size == 4)
        EigenStuff4();
    else
        EigenStuffN();
}

void zCMgcEigenD::DecrSortEigenStuff2()
{
    EigenStuff2();
    DecreasingSort(size, diag, mat);
}

void zCMgcEigenD::DecrSortEigenStuff3()
{
    EigenStuff3();
    DecreasingSort(size, diag, mat);
}

void zCMgcEigenD::DecrSortEigenStuff4()
{
    EigenStuff4();
    DecreasingSort(size, diag, mat);
}

void zCMgcEigenD::DecrSortEigenStuffN()
{
    EigenStuffN();
    DecreasingSort(size, diag, mat);
}

void zCMgcEigenD::DecrSortEigenStuff()
{
    EigenStuff();
    DecreasingSort(size, diag, mat);
}

void zCMgcEigenD::IncrSortEigenStuff2()
{
    EigenStuff2();
    IncreasingSort(size, diag, mat);
}

void zCMgcEigenD::IncrSortEigenStuff3()
{
    EigenStuff3();
    IncreasingSort(size, diag, mat);
}

void zCMgcEigenD::IncrSortEigenStuff4()
{
    EigenStuff4();
    IncreasingSort(size, diag, mat);
}

void zCMgcEigenD::IncrSortEigenStuffN()
{
    EigenStuffN();
    IncreasingSort(size, diag, mat);
}

void zCMgcEigenD::IncrSortEigenStuff()
{
    EigenStuff();
    IncreasingSort(size, diag, mat);
}

int zCMgcEigenD::Number(unsigned int code)
{
    int result = -1;
    while (code) {
        code >>= 1;
        ++result;
    }
    return result;
}

int zCMgcEigenD::verbose = 0;
unsigned int zCMgcEigenD::error = 0;
const unsigned int zCMgcEigenD::invalid_size = 1;
const unsigned int zCMgcEigenD::allocation_failed = 2;
const unsigned int zCMgcEigenD::ql_exceeded = 4;
const char* zCMgcEigenD::message[5] = {
    "invalid size", "allocation failed", "QL iteration limit exceeded", "", ""
};

void zCMgcEigenD::Report(unsigned int code)
{
    const int index = Number(code);
    if (verbose)
        fprintf(stdout, "zCMgcEigenD: %s\n", message[index]);
    else {
        FILE* file = fopen("eigen.err", "w");
        if (file) {
            fprintf(file, "zCMgcEigenD: %s\n", message[index]);
            fclose(file);
        }
    }
    error |= code;
}

void zCMgcEigenD::Report(std::ostream& stream)
{
    for (unsigned int code = 1; code; code <<= 1) {
        if (error & code)
            stream << "zCMgcEigenD: " << message[Number(code)] << std::flush;
    }
    error = 0;
}
