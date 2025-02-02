/*
 * Copyright (C) 2020 AdvanceSoft Corporation
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */

#include "nnp_symm_func_behler.h"

#define CHI0_THR  NNPREAL(1.0e-6)

SymmFuncBehler::SymmFuncBehler(int numElems, bool tanhCutFunc, bool elemWeight, int sizeRad, int sizeAng,
                               nnpreal rcutRad, nnpreal rcutAng) : SymmFunc(numElems, tanhCutFunc, elemWeight)
{
    if (sizeRad < 1)
    {
        stop_by_error("size of radius basis is not positive.");
    }

    if (sizeAng < 0)
    {
        stop_by_error("size of angle basis is negative.");
    }

    if (rcutRad <= ZERO)
    {
        stop_by_error("cutoff for radius is not positive.");
    }

    if (sizeAng > 0 && rcutAng <= ZERO)
    {
        stop_by_error("cutoff for angle is not positive.");
    }

    this->sizeRad = sizeRad;
    this->sizeAng = sizeAng;

    if (this->elemWeight)
    {
        this->numRadBasis = this->sizeRad;
        this->numAngBasis = this->sizeAng * 2;
    }
    else
    {
        this->numRadBasis = this->sizeRad * this->numElems;
        this->numAngBasis = this->sizeAng * 2 * (this->numElems * (this->numElems + 1) / 2);
    }

    this->numBasis = this->numRadBasis + this->numAngBasis;

    this->rcutRad = rcutRad;
    this->rcutAng = rcutAng;

    this->radiusEta   = nullptr;
    this->radiusShift = nullptr;

    this->angleMod   = false;
    this->angleEta   = nullptr;
    this->angleZeta  = nullptr;
    this->angleShift = nullptr;
}

SymmFuncBehler::~SymmFuncBehler()
{
    // NOP
}

void SymmFuncBehler::calculate(int numNeighbor, int* elemNeighbor, nnpreal** posNeighbor,
                               nnpreal* symmData, nnpreal* symmDiff)
{
    if (elemNeighbor == nullptr || posNeighbor == nullptr)
    {
        stop_by_error("neighbor is null.");
    }

    if (symmData == nullptr)
    {
        stop_by_error("symmData is null.");
    }

    if (symmDiff == nullptr)
    {
        stop_by_error("symmDiff is null.");
    }

    // define varialbes
    const int numFree = 3 * numNeighbor;

    int ineigh1, ineigh2;
    int mneigh;

    int jelem1, jelem2;
    int ifree1, ifree2;

    int jbase, kbase;

    nnpreal x1, x2, x3;
    nnpreal y1, y2, y3;
    nnpreal z1, z2, z3;
    nnpreal r1, r2, r3, rr3;

    nnpreal zeta1[this->sizeAng];

    int     ilambda;
    nnpreal lambda;

    nnpreal fc1, fc2, fc3;
    nnpreal dfc1dr1, dfc2dr2, dfc3dr3;
    nnpreal dfc1dx1, dfc2dx2, dfc3dx3;
    nnpreal dfc1dy1, dfc2dy2, dfc3dy3;
    nnpreal dfc1dz1, dfc2dz2, dfc3dz3;

    nnpreal fc0;
    nnpreal dfc0dx1, dfc0dx2, dfc0dx3;
    nnpreal dfc0dy1, dfc0dy2, dfc0dy3;
    nnpreal dfc0dz1, dfc0dz2, dfc0dz3;

    nnpreal psi;
    nnpreal dpsidx1, dpsidx2;
    nnpreal dpsidy1, dpsidy2;
    nnpreal dpsidz1, dpsidz2;

    nnpreal chi0;

    nnpreal zanum1, zanum2;
    nnpreal zscale;

    nnpreal fact0, fact1, fact2;

    // initialize symmetry functions
    for (int ibase = 0; ibase < this->numBasis; ++ibase)
    {
        symmData[ibase] = ZERO;
    }

    for (ifree1 = 0; ifree1 < numFree; ++ifree1)
    {
        for (int ibase = 0; ibase < this->numBasis; ++ibase)
        {
            symmDiff[ibase + ifree1 * this->numBasis] = ZERO;
        }
    }

    if (numNeighbor < 1)
    {
        return;
    }

    // radial part
    for (ineigh1 = 0; ineigh1 < numNeighbor; ++ineigh1)
    {
        r1 = posNeighbor[ineigh1][0];
        if (r1 >= this->rcutRad)
        {
            continue;
        }

        x1 = posNeighbor[ineigh1][1];
        y1 = posNeighbor[ineigh1][2];
        z1 = posNeighbor[ineigh1][3];

        ifree1 = 3 * ineigh1;

        if (this->elemWeight)
        {
            jelem1 = 0;
            zanum1 = (nnpreal) elemNeighbor[ineigh1];
        }
        else
        {
            jelem1 = elemNeighbor[ineigh1];
            zanum1 = ONE;
        }

        jbase  = jelem1 * this->sizeRad;
        zscale = zanum1;

        fc1     = posNeighbor[ineigh1][4];
        dfc1dr1 = posNeighbor[ineigh1][5];
        dfc1dx1 = x1 / r1 * dfc1dr1;
        dfc1dy1 = y1 / r1 * dfc1dr1;
        dfc1dz1 = z1 / r1 * dfc1dr1;

        #pragma omp simd
        for (int imode = 0; imode < this->sizeRad; ++imode)
        {
            const nnpreal eta = this->radiusEta  [imode];
            const nnpreal rs  = this->radiusShift[imode];

            const nnpreal dr  = r1 - rs;
            const nnpreal rr  = dr * dr;

            const nnpreal gau     = exp(-eta * rr);
            const nnpreal coef0   = -NNPREAL(2.0) * eta * dr / r1 * gau;
            const nnpreal dgaudx1 = x1 * coef0;
            const nnpreal dgaudy1 = y1 * coef0;
            const nnpreal dgaudz1 = z1 * coef0;

            const nnpreal g     = zscale * gau * fc1;
            const nnpreal dgdx1 = zscale * (dgaudx1 * fc1 + gau * dfc1dx1);
            const nnpreal dgdy1 = zscale * (dgaudy1 * fc1 + gau * dfc1dy1);
            const nnpreal dgdz1 = zscale * (dgaudz1 * fc1 + gau * dfc1dz1);

            const int ibase = imode + jbase;

            symmData[ibase] += g;

            symmDiff[ibase + (ifree1 + 0) * this->numBasis] += dgdx1;
            symmDiff[ibase + (ifree1 + 1) * this->numBasis] += dgdy1;
            symmDiff[ibase + (ifree1 + 2) * this->numBasis] += dgdz1;
        }
    }

    if (numNeighbor < 2 || this->sizeAng < 1)
    {
        return;
    }

    // angular part
    for (int imode = 0; imode < this->sizeAng; ++imode)
    {
        const nnpreal zeta = this->angleZeta[imode];
        zeta1[imode] = pow(NNPREAL(2.0), ONE - zeta);
    }

    for (ineigh2 = 0; ineigh2 < numNeighbor; ++ineigh2)
    {
        r2 = posNeighbor[ineigh2][0];
        if (r2 >= this->rcutAng)
        {
            continue;
        }

        x2 = posNeighbor[ineigh2][1];
        y2 = posNeighbor[ineigh2][2];
        z2 = posNeighbor[ineigh2][3];

        ifree2 = 3 * ineigh2;

        if (this->elemWeight)
        {
            jelem2 = 0;
            zanum2 = (nnpreal) elemNeighbor[ineigh2];
            mneigh = ineigh2;
        }
        else
        {
            jelem2 = elemNeighbor[ineigh2];
            zanum2 = ONE;
            mneigh = numNeighbor;
        }

        fc2     = posNeighbor[ineigh2][6];
        dfc2dr2 = posNeighbor[ineigh2][7];
        dfc2dx2 = x2 / r2 * dfc2dr2;
        dfc2dy2 = y2 / r2 * dfc2dr2;
        dfc2dz2 = z2 / r2 * dfc2dr2;

        for (ineigh1 = 0; ineigh1 < mneigh; ++ineigh1)
        {
            r1 = posNeighbor[ineigh1][0];
            if (r1 >= this->rcutAng)
            {
                continue;
            }

            x1 = posNeighbor[ineigh1][1];
            y1 = posNeighbor[ineigh1][2];
            z1 = posNeighbor[ineigh1][3];

            ifree1 = 3 * ineigh1;

            if (this->elemWeight)
            {
                jelem1 = 0;
                zanum1 = (nnpreal) elemNeighbor[ineigh1];
                zscale = sqrt(zanum1 * zanum2);
            }
            else
            {
                jelem1 = elemNeighbor[ineigh1];
                zanum1 = ONE;
                zscale = ONE;

                if (jelem1 > jelem2 || (jelem1 == jelem2 && ineigh1 >= ineigh2))
                {
                    continue;
                }
            }

            kbase  = (jelem1 + jelem2 * (jelem2 + 1) / 2) * 2 * this->sizeAng;

            fc1     = posNeighbor[ineigh1][6];
            dfc1dr1 = posNeighbor[ineigh1][7];
            dfc1dx1 = x1 / r1 * dfc1dr1;
            dfc1dy1 = y1 / r1 * dfc1dr1;
            dfc1dz1 = z1 / r1 * dfc1dr1;

            if (this->angleMod)
            {
                fc0 = fc1 * fc2;
                dfc0dx1 = dfc1dx1 * fc2;
                dfc0dy1 = dfc1dy1 * fc2;
                dfc0dz1 = dfc1dz1 * fc2;
                dfc0dx2 = fc1 * dfc2dx2;
                dfc0dy2 = fc1 * dfc2dy2;
                dfc0dz2 = fc1 * dfc2dz2;
            }

            else
            {
                x3  = x1 - x2;
                y3  = y1 - y2;
                z3  = z1 - z2;
                rr3 = x3 * x3 + y3 * y3 + z3 * z3;

                if (rr3 >= this->rcutAng * this->rcutAng)
                {
                    continue;
                }

                r3 = sqrt(rr3);

                this->cutoffFunction(&fc3, &dfc3dr3, r3, this->rcutAng);
                dfc3dx3 = x3 / r3 * dfc3dr3;
                dfc3dy3 = y3 / r3 * dfc3dr3;
                dfc3dz3 = z3 / r3 * dfc3dr3;

                fc0 = fc1 * fc2 * fc3;
                dfc0dx1 = dfc1dx1 * fc2 * fc3;
                dfc0dy1 = dfc1dy1 * fc2 * fc3;
                dfc0dz1 = dfc1dz1 * fc2 * fc3;
                dfc0dx2 = fc1 * dfc2dx2 * fc3;
                dfc0dy2 = fc1 * dfc2dy2 * fc3;
                dfc0dz2 = fc1 * dfc2dz2 * fc3;
                dfc0dx3 = fc1 * fc2 * dfc3dx3;
                dfc0dy3 = fc1 * fc2 * dfc3dy3;
                dfc0dz3 = fc1 * fc2 * dfc3dz3;
            }

            psi     = (x1 * x2 + y1 * y2 + z1 * z2) / r1 / r2;
            fact0   = ONE / r1 / r2;
            fact1   = psi / r1 / r1;
            fact2   = psi / r2 / r2;
            dpsidx1 = fact0 * x2 - fact1 * x1;
            dpsidy1 = fact0 * y2 - fact1 * y1;
            dpsidz1 = fact0 * z2 - fact1 * z1;
            dpsidx2 = fact0 * x1 - fact2 * x2;
            dpsidy2 = fact0 * y1 - fact2 * y2;
            dpsidz2 = fact0 * z1 - fact2 * z2;

            for (ilambda = 0; ilambda < 2; ++ilambda)
            {
                lambda = (ilambda == 0) ? ONE : (-ONE);

                chi0 = ONE + lambda * psi;
                if (chi0 < CHI0_THR)
                {
                    continue;
                }

                jbase = ilambda * this->sizeAng;

                if (this->angleMod)
                {
                    #pragma omp simd
                    for (int imode = 0; imode < this->sizeAng; ++imode)
                    {
                        const nnpreal eta   = this->angleEta  [imode];
                        const nnpreal rs    = this->angleShift[imode];
                        const nnpreal zeta  = this->angleZeta [imode];
                        const nnpreal zeta0 = zeta1[imode];

                        const nnpreal chi      = zeta0 * pow(chi0, zeta);
                        const nnpreal dchidpsi = zeta * lambda * chi / chi0;

                        const nnpreal rr = (r1 - rs) * (r1 - rs) + (r2 - rs) * (r2 - rs);

                        const nnpreal gau     = exp(-eta * rr);
                        const nnpreal coef0   = -NNPREAL(2.0) * eta * gau;
                        const nnpreal coef1   = coef0 * (r1 - rs) / r1;
                        const nnpreal coef2   = coef0 * (r2 - rs) / r2;
                        const nnpreal dgaudx1 = coef1 * x1;
                        const nnpreal dgaudy1 = coef1 * y1;
                        const nnpreal dgaudz1 = coef1 * z1;
                        const nnpreal dgaudx2 = coef2 * x2;
                        const nnpreal dgaudy2 = coef2 * y2;
                        const nnpreal dgaudz2 = coef2 * z2;

                        const nnpreal g     = zscale * chi * gau * fc0;
                        const nnpreal dgdx1 = zscale * (dchidpsi * dpsidx1 * gau * fc0 + chi * dgaudx1 * fc0 + chi * gau * dfc0dx1);
                        const nnpreal dgdy1 = zscale * (dchidpsi * dpsidy1 * gau * fc0 + chi * dgaudy1 * fc0 + chi * gau * dfc0dy1);
                        const nnpreal dgdz1 = zscale * (dchidpsi * dpsidz1 * gau * fc0 + chi * dgaudz1 * fc0 + chi * gau * dfc0dz1);
                        const nnpreal dgdx2 = zscale * (dchidpsi * dpsidx2 * gau * fc0 + chi * dgaudx2 * fc0 + chi * gau * dfc0dx2);
                        const nnpreal dgdy2 = zscale * (dchidpsi * dpsidy2 * gau * fc0 + chi * dgaudy2 * fc0 + chi * gau * dfc0dy2);
                        const nnpreal dgdz2 = zscale * (dchidpsi * dpsidz2 * gau * fc0 + chi * dgaudz2 * fc0 + chi * gau * dfc0dz2);

                        const int ibase = this->numRadBasis + imode + jbase + kbase;

                        symmData[ibase] += g;

                        symmDiff[ibase + (ifree1 + 0) * this->numBasis] += dgdx1;
                        symmDiff[ibase + (ifree1 + 1) * this->numBasis] += dgdy1;
                        symmDiff[ibase + (ifree1 + 2) * this->numBasis] += dgdz1;

                        symmDiff[ibase + (ifree2 + 0) * this->numBasis] += dgdx2;
                        symmDiff[ibase + (ifree2 + 1) * this->numBasis] += dgdy2;
                        symmDiff[ibase + (ifree2 + 2) * this->numBasis] += dgdz2;
                    }
                }

                else
                {
                    #pragma omp simd
                    for (int imode = 0; imode < this->sizeAng; ++imode)
                    {
                        const nnpreal eta   = this->angleEta  [imode];
                        const nnpreal rs    = this->angleShift[imode];
                        const nnpreal zeta  = this->angleZeta [imode];
                        const nnpreal zeta0 = zeta1[imode];

                        const nnpreal chi      = zeta0 * pow(chi0, zeta);
                        const nnpreal dchidpsi = zeta * lambda * chi / chi0;

                        const nnpreal rr = (r1 - rs) * (r1 - rs) + (r2 - rs) * (r2 - rs) + (r3 - rs) * (r3 - rs);

                        const nnpreal gau     = exp(-eta * rr);
                        const nnpreal coef0   = -NNPREAL(2.0) * eta * gau;
                        const nnpreal coef1   = coef0 * (r1 - rs) / r1;
                        const nnpreal coef2   = coef0 * (r2 - rs) / r2;
                        const nnpreal coef3   = coef0 * (r3 - rs) / r3;
                        const nnpreal dgaudx1 = coef1 * x1;
                        const nnpreal dgaudy1 = coef1 * y1;
                        const nnpreal dgaudz1 = coef1 * z1;
                        const nnpreal dgaudx2 = coef2 * x2;
                        const nnpreal dgaudy2 = coef2 * y2;
                        const nnpreal dgaudz2 = coef2 * z2;
                        const nnpreal dgaudx3 = coef3 * x3;
                        const nnpreal dgaudy3 = coef3 * y3;
                        const nnpreal dgaudz3 = coef3 * z3;

                        const nnpreal g     = zscale * (chi * gau * fc0);
                        const nnpreal dgdx1 = zscale * (dchidpsi * dpsidx1 * gau * fc0 + chi * dgaudx1 * fc0 + chi * gau * dfc0dx1);
                        const nnpreal dgdy1 = zscale * (dchidpsi * dpsidy1 * gau * fc0 + chi * dgaudy1 * fc0 + chi * gau * dfc0dy1);
                        const nnpreal dgdz1 = zscale * (dchidpsi * dpsidz1 * gau * fc0 + chi * dgaudz1 * fc0 + chi * gau * dfc0dz1);
                        const nnpreal dgdx2 = zscale * (dchidpsi * dpsidx2 * gau * fc0 + chi * dgaudx2 * fc0 + chi * gau * dfc0dx2);
                        const nnpreal dgdy2 = zscale * (dchidpsi * dpsidy2 * gau * fc0 + chi * dgaudy2 * fc0 + chi * gau * dfc0dy2);
                        const nnpreal dgdz2 = zscale * (dchidpsi * dpsidz2 * gau * fc0 + chi * dgaudz2 * fc0 + chi * gau * dfc0dz2);
                        const nnpreal dgdx3 = zscale * (chi * dgaudx3 * fc0 + chi * gau * dfc0dx3);
                        const nnpreal dgdy3 = zscale * (chi * dgaudy3 * fc0 + chi * gau * dfc0dy3);
                        const nnpreal dgdz3 = zscale * (chi * dgaudz3 * fc0 + chi * gau * dfc0dz3);

                        const int ibase = this->numRadBasis + imode + jbase + kbase;

                        symmData[ibase] += g;

                        symmDiff[ibase + (ifree1 + 0) * this->numBasis] += dgdx1 + dgdx3;
                        symmDiff[ibase + (ifree1 + 1) * this->numBasis] += dgdy1 + dgdy3;
                        symmDiff[ibase + (ifree1 + 2) * this->numBasis] += dgdz1 + dgdz3;

                        symmDiff[ibase + (ifree2 + 0) * this->numBasis] += dgdx2 - dgdx3;
                        symmDiff[ibase + (ifree2 + 1) * this->numBasis] += dgdy2 - dgdy3;
                        symmDiff[ibase + (ifree2 + 2) * this->numBasis] += dgdz2 - dgdz3;
                    }
                }
            }
        }
    }
}
