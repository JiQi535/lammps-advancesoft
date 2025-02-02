/*
 * Copyright (C) 2022 AdvanceSoft Corporation
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef NNP_SYMM_FUNC_GPU_H_
#define NNP_SYMM_FUNC_GPU_H_

#include "nnp_common.h"
#include "nnp_symm_func.h"

// sizeof(gint) has to equal sizeof(nnpreal), to avoid bank conflict on shared memory
#ifdef _NNP_SINGLE
#define gint  int
#else
#define gint  long long
#endif

#define MAX_ELEMENT_PAIRS   10
#define MAX_ELEMENT_PAIRS3  30

#define SYMMFUNC_DIRECT_COPY
#define SYMMDIFF_HIDDEN

class SymmFuncGPU : public SymmFunc
{
public:
    SymmFuncGPU(int numElems, bool tanhCutFunc, bool elemWeight,
                int sizeRad, int sizeAng, nnpreal rcutRad, nnpreal rcutAng, int cutoffMode);

    virtual ~SymmFuncGPU() override;

    void calculate(int numNeighbor, int* elemNeighbor, nnpreal** posNeighbor,
                   nnpreal* symmData, nnpreal* symmDiff) override
    {
        int idxNeighbor = 0;

        this->calculate(1, &numNeighbor, &idxNeighbor, &elemNeighbor, &posNeighbor, symmData, symmDiff);
    }

    void calculate(int lenAtoms, int* numNeighbor, int* idxNeighbor, int** elemNeighbor, nnpreal*** posNeighbor,
                   nnpreal* symmData, nnpreal* symmDiff) override;

    nnpreal* getSymmGrad() override
    {
        return this->symmGrad;
    }

    void allocHiddenDiff(int maxAtoms, int fullNeigh) override;

    void driveHiddenDiff(int lenAtoms, int* numNeighbor, int* idxNeighbor, nnpreal* forceData) override;

    void setMaxThreadsPerBlock(int maxThreadsPerBlock)
    {
        if (maxThreadsPerBlock > 0)
        {
            this->maxThreadsPerBlock = maxThreadsPerBlock;
        }
    }

    int getNumRadBasis() const
    {
        return this->numRadBasis;
    }

    int getNumAngBasis() const
    {
        return this->numAngBasis;
    }

protected:
    int sizeRad;
    int sizeAng;

    int numRadBasis;
    int numAngBasis;

    nnpreal rcutRad;
    nnpreal rcutAng;

    int sizePosNeighbor;

    int*     numNeighs;
    int*     numNeighs_d;
    int*     idxNeighs;
    int*     idxNeighs_d;
    gint*    elementAll;
    gint*    elementAll_d;
    nnpreal* posNeighborAll;
    nnpreal* posNeighborAll_d;
    nnpreal* symmDataSum;
    nnpreal* symmDataSum_d;
    nnpreal* symmDataAll_d;
    nnpreal* symmDiffAll;
    nnpreal* symmDiffAll_d;
    nnpreal* symmDiffFull_d;
    nnpreal* symmGrad;
    nnpreal* symmGrad_d;
    nnpreal* forceData;
    nnpreal* forceData_d;

    virtual void calculateRadial(dim3 grid, dim3 block, int idxNeigh) = 0;

    virtual void calculateAnglarElemWeight(dim3 grid, dim3 block, size_t sizeShared, int idxNeigh) = 0;

    virtual void calculateAnglarNotElemWeight(dim3 grid, dim3 block, size_t sizeShared, int idxNeigh, int dimBasis) = 0;

private:
    int maxThreadsPerBlock;

    int sizeLenAtoms;
    int sizeMaxAtoms;
    int sizeTotNeigh1;
    int sizeTotNeigh2;
    int sizeFullNeigh;

    void getSizeOfModeBatchs(int* numModeBatchs, int* modesPerBatch, int sizeMode, int maxNeigh);
};

inline void SymmFuncGPU::getSizeOfModeBatchs(int* numModeBatchs, int* modesPerBatch, int sizeMode, int maxNeigh)
{
    numModeBatchs[0] = (sizeMode * maxNeigh - 1) / this->maxThreadsPerBlock + 1;

    while (true)
    {
        if (numModeBatchs[0] < 1 || sizeMode < numModeBatchs[0])
        {
            stop_by_error("cannot resolve size of mode batchs.");
        }

        modesPerBatch[0] = sizeMode / numModeBatchs[0];
        if ((sizeMode % numModeBatchs[0]) > 0) modesPerBatch[0]++;

        if ((modesPerBatch[0] * maxNeigh) <= this->maxThreadsPerBlock)
        {
            break;
        }
        else
        {
            numModeBatchs[0]++;
        }
    }
}

#endif /* NNP_SYMM_FUNC_GPU_H_ */
