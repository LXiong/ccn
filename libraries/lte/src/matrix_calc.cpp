// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.


#include "matrix_calc.h"

// /**
// FUNCTION   :: GetTransposeMatrix
// LAYER      :: PHY
// PURPOSE    :: Get transposed matrix
// PARAMETERS ::
// + org : Cmat : Matrix to transpose
// RETURN     :: Cmat : Transposed matrix
// **/
Cmat GetTransposeMatrix(const Cmat& org)
{
    assert(org.size() == 4);

    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = org[m11];
    ret[m12] = org[m21];
    ret[m21] = org[m12];
    ret[m22] = org[m22];
    return ret;
}

// /**
// FUNCTION   :: GetConjugateTransposeMatrix
// LAYER      :: PHY
// PURPOSE    :: Get Conjugate transposed matrix
// PARAMETERS ::
// + org : Cmat : Matrix to transpose
// RETURN     :: Cmat : Conjugate transposed matrix
// **/
Cmat GetConjugateTransposeMatrix(const Cmat& org)
{
    assert(org.size() == 4);

    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = conj(org[m11]);
    ret[m12] = conj(org[m21]);
    ret[m21] = conj(org[m12]);
    ret[m22] = conj(org[m22]);
    return ret;
}

// /**
// FUNCTION   :: MulMatrix
// LAYER      :: PHY
// PURPOSE    :: Multiply 2 matrices
// PARAMETERS ::
// + m1 : const Cmat& : Left term matrix
// + m2 : const Cmat& : Right term matrix
// RETURN     :: Cmat : Multiplied matrix
// **/
Cmat MulMatrix(const Cmat& m1, const Cmat& m2)
{
    assert(m1.size() == 4);
    assert(m2.size() == 4);

    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = m1[m11] * m2[m11] + m1[m12] * m2[m21];
    ret[m12] = m1[m11] * m2[m12] + m1[m12] * m2[m22];
    ret[m21] = m1[m21] * m2[m11] + m1[m22] * m2[m21];
    ret[m22] = m1[m21] * m2[m12] + m1[m22] * m2[m22];
    return ret;
}

// /**
// FUNCTION   :: SumMatrix
// LAYER      :: PHY
// PURPOSE    :: Sum 2 matrices
// PARAMETERS ::
// + m1 : const Cmat& : Left term matrix
// + m2 : const Cmat& : Right term matrix
// RETURN     :: Cmat : Multiplied matrix
// **/
Cmat SumMatrix(const Cmat& m1, const Cmat& m2)
{
    assert(m1.size() == 4);
    assert(m2.size() == 4);

    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = m1[m11] + m2[m11];
    ret[m12] = m1[m12] + m2[m12];
    ret[m21] = m1[m21] + m2[m21];
    ret[m22] = m1[m22] + m2[m22];
    return ret;
}

// /**
// FUNCTION   :: GetInvertMatrix
// LAYER      :: PHY
// PURPOSE    :: Get inverted matrix
// PARAMETERS ::
// + org : const Cmat& : Matrix to invert
// RETURN     :: Cmat : Inverted matrix
// **/
Cmat GetInvertMatrix(const Cmat& org)
{
    assert(org.size() == 4);

    Dcomp det_err(0.0, 0.0);
    Dcomp det_org = org[m11] * org[m22] - org[m12] * org[m21];

    assert(det_org != det_err);

    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = org[m22] / det_org;
    ret[m12] = (-1.0) * org[m12] / det_org;
    ret[m21] = (-1.0) * org[m21] / det_org;
    ret[m22] = org[m11] / det_org;
    return ret;
}

// /**
// FUNCTION   :: GetDiagMatrix
// LAYER      :: PHY
// PURPOSE    :: Get diagonal matrix
// PARAMETERS ::
// + e1 : const Dcomp& : (0,0) element
// + e2 : const Dcomp& : (1,1) element
// RETURN     :: Cmat : Diagonal matrix
// **/
Cmat GetDiagMatrix(const Dcomp& e1, const Dcomp& e2)
{
    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = e1;
    ret[m22] = e2;
    return ret;
}

// /**
// FUNCTION   :: GetDiagMatrix
// LAYER      :: PHY
// PURPOSE    :: Get diagonal matrix
// PARAMETERS ::
// + e1 : double : (0,0) element
// + e2 : double : (1,1) element
// RETURN     :: Cmat : Diagonal matrix
// **/
Cmat GetDiagMatrix(double e1, double e2)
{
    Cmat ret(LTE_DEFAULT_DCOMP,4);
    ret[m11] = e1;
    ret[m22] = e2;
    return ret;
}
