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

/*
 */

#ifndef _MATRIX_CALC_H_
#define _MATRIX_CALC_H_

#include <assert.h>

#include <complex>
#include <valarray>

///////////////////////////////////////////////////////////////
// typedef
///////////////////////////////////////////////////////////////
typedef std::complex < double > Dcomp;
typedef std::valarray < Dcomp > Cmat;

///////////////////////////////////////////////////////////////
// typedef enum
///////////////////////////////////////////////////////////////

// /**
// ENUM        :: MatrixIndex2x2
// DESCRIPTION :: Matrix Index(2x2 only)
// **/
typedef enum
{
    m11,
    m12,
    m21,
    m22,
    numberOfElements2x2
}MatrixIndex2x2;


///////////////////////////////////////////////////////////////
// default value
///////////////////////////////////////////////////////////////
const Dcomp LTE_DEFAULT_DCOMP(0.0, 0.0);

//--------------------------------------------------------------------------
//  Utility functions for calculation of 2x2 complex matrix
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: GetTransposeMatrix
// LAYER      :: PHY
// PURPOSE    :: Get transposed matrix
// PARAMETERS ::
// + org : Cmat : Matrix to transpose
// RETURN     :: Cmat : Transposed matrix
// **/
Cmat GetTransposeMatrix(const Cmat& org);

// /**
// FUNCTION   :: GetConjugateTransposeMatrix
// LAYER      :: PHY
// PURPOSE    :: Get Conjugate transposed matrix
// PARAMETERS ::
// + org : Cmat : Matrix to transpose
// RETURN     :: Cmat : Conjugate transposed matrix
// **/
Cmat GetConjugateTransposeMatrix(const Cmat& org);

// /**
// FUNCTION   :: MulMatrix
// LAYER      :: PHY
// PURPOSE    :: Multiply 2 matrices
// PARAMETERS ::
// + m1 : const Cmat& : Left term matrix
// + m2 : const Cmat& : Right term matrix
// RETURN     :: Cmat : Multiplied matrix
// **/
Cmat MulMatrix(const Cmat& m1, const Cmat& m2);

// /**
// FUNCTION   :: SumMatrix
// LAYER      :: PHY
// PURPOSE    :: Sum 2 matrices
// PARAMETERS ::
// + m1 : const Cmat& : Left term matrix
// + m2 : const Cmat& : Right term matrix
// RETURN     :: Cmat : Multiplied matrix
// **/
Cmat SumMatrix(const Cmat& m1, const Cmat& m2);

// /**
// FUNCTION   :: GetInvertMatrix
// LAYER      :: PHY
// PURPOSE    :: Get inverted matrix
// PARAMETERS ::
// + org : const Cmat& : Matrix to invert
// RETURN     :: Cmat : Inverted matrix
// **/
Cmat GetInvertMatrix(const Cmat& org);

// /**
// FUNCTION   :: GetDiagMatrix
// LAYER      :: PHY
// PURPOSE    :: Get diagonal matrix
// PARAMETERS ::
// + e1 : const Dcomp& : (0,0) element
// + e2 : const Dcomp& : (1,1) element
// RETURN     :: Cmat : Diagonal matrix
// **/
Cmat GetDiagMatrix(const Dcomp& e1, const Dcomp& e2);

// /**
// FUNCTION   :: GetDiagMatrix
// LAYER      :: PHY
// PURPOSE    :: Get diagonal matrix
// PARAMETERS ::
// + e1 : double : (0,0) element
// + e2 : double : (1,1) element
// RETURN     :: Cmat : Diagonal matrix
// **/
Cmat GetDiagMatrix(double e1, double e2);


#endif /* _MATRIX_CALC_H_ */
