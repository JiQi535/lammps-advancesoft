/*
 * Copyright (C) 2020 AdvanceSoft Corporation
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef NNP_SYMM_FUNC_CHEBYSHEV_H_
#define NNP_SYMM_FUNC_CHEBYSHEV_H_

#include "nnp_common.h"
#include "nnp_symm_func.h"

#define CHEBYSHEV_UNROLL4
#define CHEBYSHEV_TRIGONO

#define SMALL_ANG  NNPREAL(0.00001)

class SymmFuncChebyshev : public SymmFunc
{
public:
    SymmFuncChebyshev(int numElems, bool tanhCutFunc, bool elemWeight,
                      int sizeRad, int sizeAng, nnpreal rcutRad, nnpreal rcutAng);

    virtual ~SymmFuncChebyshev() override;

    void calculate(int numNeighbor, int* elemNeighbor, nnpreal** posNeighbor,
                   nnpreal* symmData, nnpreal* symmDiff) override;

    int getNumRadBasis() const
    {
        return this->numRadBasis;
    }

    int getNumAngBasis() const
    {
        return this->numAngBasis;
    }

private:
    int sizeRad;
    int sizeAng;

    int numRadBasis;
    int numAngBasis;

    nnpreal rcutRad;
    nnpreal rcutAng;

#ifdef CHEBYSHEV_TRIGONO
    void chebyshevTrigonometric(nnpreal* t, nnpreal* dt, nnpreal r, int n) const;
#else
    void chebyshevFunction(nnpreal* t, nnpreal* dt, nnpreal s, int n) const;
#endif
};

#ifdef CHEBYSHEV_TRIGONO
inline void SymmFuncChebyshev::chebyshevTrigonometric(nnpreal* t, nnpreal* dt, nnpreal r, int n) const
{
    nnpreal k = (nnpreal) n;
    t [0]     = cos(k * r);
    dt[0]     = r < SMALL_ANG ? (k * k * (ONE - (k * k - ONE) / NNPREAL(6.0) * r * r))
              : (k * sin(k * r) / sin(r));
}

#else
inline void SymmFuncChebyshev::chebyshevFunction(nnpreal* t, nnpreal* dt, nnpreal s, int n) const
{
    int i;
    int m;

    t [0] = ONE;
    t [1] = s;
    dt[0] = ZERO;
    dt[1] = ONE;

#if defined(CHEBYSHEV_NOT_UNROLL)
    m = 2;

#elif defined(CHEBYSHEV_UNROLL2)
    m = n - ((n - 2) % 2);

    for (i = 2; i < m; i += 2)
    {
        t [i    ] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        t [i + 1] = NNPREAL(2.0) * s * t[i    ] - t[i - 1];
        dt[i    ] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
        dt[i + 1] = NNPREAL(2.0) * (s * dt[i    ] + t[i    ]) - dt[i - 1];
    }

#elif defined(CHEBYSHEV_UNROLL4)
    m = n - ((n - 2) % 4);

    for (i = 2; i < m; i += 4)
    {
        t [i    ] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        t [i + 1] = NNPREAL(2.0) * s * t[i    ] - t[i - 1];
        t [i + 2] = NNPREAL(2.0) * s * t[i + 1] - t[i    ];
        t [i + 3] = NNPREAL(2.0) * s * t[i + 2] - t[i + 1];
        dt[i    ] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
        dt[i + 1] = NNPREAL(2.0) * (s * dt[i    ] + t[i    ]) - dt[i - 1];
        dt[i + 2] = NNPREAL(2.0) * (s * dt[i + 1] + t[i + 1]) - dt[i    ];
        dt[i + 3] = NNPREAL(2.0) * (s * dt[i + 2] + t[i + 2]) - dt[i + 1];
    }

#elif defined(CHEBYSHEV_UNROLL6)
    int l = n - ((n - 2) % 6);

    for (i = 2; i < l; i += 6)
    {
        t [i    ] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        t [i + 1] = NNPREAL(2.0) * s * t[i    ] - t[i - 1];
        t [i + 2] = NNPREAL(2.0) * s * t[i + 1] - t[i    ];
        t [i + 3] = NNPREAL(2.0) * s * t[i + 2] - t[i + 1];
        t [i + 4] = NNPREAL(2.0) * s * t[i + 3] - t[i + 2];
        t [i + 5] = NNPREAL(2.0) * s * t[i + 4] - t[i + 3];
        dt[i    ] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
        dt[i + 1] = NNPREAL(2.0) * (s * dt[i    ] + t[i    ]) - dt[i - 1];
        dt[i + 2] = NNPREAL(2.0) * (s * dt[i + 1] + t[i + 1]) - dt[i    ];
        dt[i + 3] = NNPREAL(2.0) * (s * dt[i + 2] + t[i + 2]) - dt[i + 1];
        dt[i + 4] = NNPREAL(2.0) * (s * dt[i + 3] + t[i + 3]) - dt[i + 2];
        dt[i + 5] = NNPREAL(2.0) * (s * dt[i + 4] + t[i + 4]) - dt[i + 3];
    }

    m = n - ((n - l) % 4);

    for (i = l; i < m; i += 4)
    {
        t [i    ] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        t [i + 1] = NNPREAL(2.0) * s * t[i    ] - t[i - 1];
        t [i + 2] = NNPREAL(2.0) * s * t[i + 1] - t[i    ];
        t [i + 3] = NNPREAL(2.0) * s * t[i + 2] - t[i + 1];
        dt[i    ] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
        dt[i + 1] = NNPREAL(2.0) * (s * dt[i    ] + t[i    ]) - dt[i - 1];
        dt[i + 2] = NNPREAL(2.0) * (s * dt[i + 1] + t[i + 1]) - dt[i    ];
        dt[i + 3] = NNPREAL(2.0) * (s * dt[i + 2] + t[i + 2]) - dt[i + 1];
    }

#elif defined(CHEBYSHEV_UNROLL8)
    int l = n - ((n - 2) % 8);

    for (i = 2; i < l; i += 8)
    {
        t [i    ] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        t [i + 1] = NNPREAL(2.0) * s * t[i    ] - t[i - 1];
        t [i + 2] = NNPREAL(2.0) * s * t[i + 1] - t[i    ];
        t [i + 3] = NNPREAL(2.0) * s * t[i + 2] - t[i + 1];
        t [i + 4] = NNPREAL(2.0) * s * t[i + 3] - t[i + 2];
        t [i + 5] = NNPREAL(2.0) * s * t[i + 4] - t[i + 3];
        t [i + 6] = NNPREAL(2.0) * s * t[i + 5] - t[i + 4];
        t [i + 7] = NNPREAL(2.0) * s * t[i + 6] - t[i + 5];
        dt[i    ] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
        dt[i + 1] = NNPREAL(2.0) * (s * dt[i    ] + t[i    ]) - dt[i - 1];
        dt[i + 2] = NNPREAL(2.0) * (s * dt[i + 1] + t[i + 1]) - dt[i    ];
        dt[i + 3] = NNPREAL(2.0) * (s * dt[i + 2] + t[i + 2]) - dt[i + 1];
        dt[i + 4] = NNPREAL(2.0) * (s * dt[i + 3] + t[i + 3]) - dt[i + 2];
        dt[i + 5] = NNPREAL(2.0) * (s * dt[i + 4] + t[i + 4]) - dt[i + 3];
        dt[i + 6] = NNPREAL(2.0) * (s * dt[i + 5] + t[i + 5]) - dt[i + 4];
        dt[i + 7] = NNPREAL(2.0) * (s * dt[i + 6] + t[i + 6]) - dt[i + 5];
    }

    m = n - ((n - l) % 4);

    for (i = l; i < m; i += 4)
    {
        t [i    ] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        t [i + 1] = NNPREAL(2.0) * s * t[i    ] - t[i - 1];
        t [i + 2] = NNPREAL(2.0) * s * t[i + 1] - t[i    ];
        t [i + 3] = NNPREAL(2.0) * s * t[i + 2] - t[i + 1];
        dt[i    ] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
        dt[i + 1] = NNPREAL(2.0) * (s * dt[i    ] + t[i    ]) - dt[i - 1];
        dt[i + 2] = NNPREAL(2.0) * (s * dt[i + 1] + t[i + 1]) - dt[i    ];
        dt[i + 3] = NNPREAL(2.0) * (s * dt[i + 2] + t[i + 2]) - dt[i + 1];
    }
#endif

    for (i = m; i < n; ++i)
    {
        t [i] = NNPREAL(2.0) * s * t[i - 1] - t[i - 2];
        dt[i] = NNPREAL(2.0) * (s * dt[i - 1] + t[i - 1]) - dt[i - 2];
    }
}
#endif

#endif /* NNP_SYMM_FUNC_CHEBYSHEV_H_ */
