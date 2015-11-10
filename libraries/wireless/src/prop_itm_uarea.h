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

// *************************************
//               uarea.h
// *************************************
// Routines for this program are taken from
// a tranlation of the FORTRAN code written by
// U.S. Department of Commerce NTIA/ITS
// Institute for Telecommunication Sciences
// *****************
// Irregular Terrain Model (ITM) (Longley-Rice)
// *************************************

#ifndef UAREA
#define UAREA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex>
#include <math.h>

#include "main.h"
#include "qualnet_error.h"

#define THIRD  (1.0 / 3.0)

using namespace std;

#ifndef _WIN32
#ifndef _AIX
typedef int __int32;
#endif
#endif

#define itm_min(x, y) (((x) < (y)) ? (x) : (y))
#define itm_max(x, y) (((x) < (y)) ? (y) : (x))

#ifdef PARALLEL //Parallel
//
// This file was reformatted using "indent" with the following options
// indent -npsl -npcs -ncs -nce -i4 -brs -br
//
// This file contains only a subset of functions that were originally
// in uarea.h
//

typedef struct static_vars {
    double wd1;
    double xd1;
    double afo;
    double qk;
    double aht;
    double xht;

    double ad;
    double rr;
    double etq;
    double h0s;

    double wls;

    bool wlos;
    bool wscat;

    double dmin;
    double xae;
} StaticVars;


struct tcomplex {
    double tcreal;
    double tcimag;
};

struct prop_type {
    double aref;
    double dist;
    double hg[2];
    double wn;
    double dh;
    double ens;
    double gme;
    double zgndreal;
    double zgndimag;
    double he[2];
    double dl[2];
    double the[2];
    __int32 kwx;
    __int32 mdp;
    // char *strkwx;  // Reason for kwx being off
};

struct propv_type {
    double sgc;
    __int32 lvar;
    __int32 mdvar;
    __int32 klim;
};

struct propa_type {
    double dlsa;
    double dx;
    double ael;
    double ak1;
    double ak2;
    double aed;
    double emd;
    double aes;
    double ems;
    double dls[2];
    double dla;
    double tha;
};

double FORTRAN_DIM(const double &x, const double &y) {
    // This performs the FORTRAN DIM function.
    // result is x-y if x is greater than y; otherwise result is 0.0
    if (x > y)
        return x - y;
    else
        return 0.0;
}

// called from adiff()
double aknfe(const double &v2) {
    double a;

    if (v2 < 5.76)
        a = 6.02 + 9.11 * sqrt(v2) - 1.27 * v2;
    else
        a = 12.953 + 4.343 * log(v2);

    return a;
}

// called from adiff()
double fht(const double &x, const double &pk) {
    double w, fhtv;

    if (x < 200.0) {
        w = -log(pk);
        if (pk < 1.0e-5 || x * pow(w, 3.0) > 5495.0) {
            fhtv = -117.0;
            if (x > 1.0)
                fhtv = 17.372 * log(x) + fhtv;
        }
        else
            fhtv = 2.5e-5 * x * x / pk - 8.686 * w - 15.0;
    }
    else {
        fhtv = 0.05751 * x - 4.343 * log(x);
        if (x < 2000.0) {
            w = 0.0134 * x * exp(-0.005 * x);
            fhtv = (1.0 - w) * fhtv + w * (17.372 * log(x) - 117.0);
        }
    }

    return fhtv;
}


// called from ascat()
double h0f(double r, double et) {
    double a[5] = { 25.0, 80.0, 177.0, 395.0, 705.0 };
    double b[5] = { 24.0, 45.0, 68.0, 80.0, 105.0 };
    double q, x;
    int it;
    double h0fv;
    it = RoundToInt(et);
    if (it <= 0) {
        it = 1;
        q = 0.0;
    }
    else if (it >= 5) {
        it = 5;
        q = 0.0;
    }
    else
        q = et - it;
    x = pow(1.0 / r, 2.0);
    h0fv = 4.343 * log((a[it - 1] * x + b[it - 1]) * x + 1.0);
    if (q != 0.0)
        h0fv =
            (1.0 - q) * h0fv + q * 4.343 * log((a[it] * x + b[it]) * x + 1.0);
    return h0fv;
}


// called from ascat()
double ahd(double td) {
    int i;
    double a[3] = { 133.4, 104.6, 71.8 };
    double b[3] = { 0.332e-3, 0.212e-3, 0.157e-3 };
    double c[3] = { -4.343, -1.086, 2.171 };
    if (td <= 10.0e3)
        i = 0;
    else if (td <= 70.0e3)
        i = 1;
    else
        i = 2;
    return a[i] + b[i] * td + c[i] * log(td);
}

// called from lrprop()
double adiff(double d, prop_type &prop, propa_type &propa, StaticVars &staticVars) {
    complex <double> prop_zgnd(prop.zgndreal, prop.zgndimag);
    //static StaticVars staticVars;
    //static double wd1, xd1, afo, qk, aht, xht; // static variables!
    double a, q, pk, ds, th, wa, ar, wd, adiffv;
    if (d == 0.0) {
        q = prop.hg[0] * prop.hg[1];
        staticVars.qk = prop.he[0] * prop.he[1] - q;
        if (prop.mdp < 0.0)
            q += 10.0;
        staticVars.wd1 = sqrt(1.0 + staticVars.qk / q);
        staticVars.xd1 = propa.dla + propa.tha / prop.gme;
        q = (1.0 - 0.8 * exp(-propa.dlsa / 50.0e3)) * prop.dh;
        q *= 0.78 * exp(-pow(q / 16.0, 0.25));
        staticVars.afo =
            itm_min(15.0,
                2.171 * log(1.0 +
                            4.77e-4 * prop.hg[0] * prop.hg[1] * prop.wn * q));
        staticVars.qk = 1.0 / abs(prop_zgnd);
        staticVars.aht = 20.0;
        staticVars.xht = 0.0;
        for (int j = 0; j < 2; ++j) {
            a = 0.5 * pow(prop.dl[j], 2.0) / prop.he[j];
            wa = pow(a * prop.wn, THIRD);
            pk = staticVars.qk / wa;
            q = (1.607 - pk) * 151.0 * wa * prop.dl[j] / a;
            staticVars.xht += q;
            staticVars.aht += fht(q, pk);
        }
        adiffv = 0.0;
    }
    else {
        th = propa.tha + d * prop.gme;
        ds = d - propa.dla;
        q = 0.0795775 * prop.wn * ds * pow(th, 2.0);
        adiffv =
            aknfe(q * prop.dl[0] / (ds + prop.dl[0])) +
            aknfe(q * prop.dl[1] / (ds + prop.dl[1]));
        a = ds / th;
        wa = pow(a * prop.wn, THIRD);
        pk = staticVars.qk / wa;
        q = (1.607 - pk) * 151.0 * wa * th + staticVars.xht;
        ar = 0.05751 * q - 4.343 * log(q) - staticVars.aht;
        q =
            (staticVars.wd1 +
             staticVars.xd1 / d) * itm_min(((1.0 - 0.8 * exp(-d / 50.0e3)) * prop.dh *
                                        prop.wn), 6283.2);
        wd = 25.1 / (25.1 + sqrt(q));
        adiffv = ar * wd + (1.0 - wd) * adiffv + staticVars.afo;
    }
    return adiffv;
}

// called from lrprop()
double ascat(double d, prop_type &prop, propa_type &propa, StaticVars &staticVars) {
    //complex <double> prop_zgnd(prop.zgndreal, prop.zgndimag);
    //static StaticVars staticVars;
    //static double ad, rr, etq, h0s; // static variables!
    double h0, r1, r2, z0, ss, et, ett, th, q;
    double ascatv;
    if (d == 0.0) {
        staticVars.ad = prop.dl[0] - prop.dl[1];
        staticVars.rr = prop.he[1] / prop.he[0];
        if (staticVars.ad < 0.0) {
            staticVars.ad = -staticVars.ad;
            staticVars.rr = 1.0 / staticVars.rr;
        }
        staticVars.etq = (5.67e-6 * prop.ens - 2.32e-3) * prop.ens + 0.031;
        staticVars.h0s = -15.0;
        ascatv = 0.0;
    }
    else {
        if (staticVars.h0s > 15.0)
            h0 = staticVars.h0s;
        else {
            th = prop.the[0] + prop.the[1] + d * prop.gme;
            r2 = 2.0 * prop.wn * th;
            r1 = r2 * prop.he[0];
            r2 *= prop.he[1];
            if (r1 < 0.2 && r2 < 0.2)
                return 1001.0;  // <==== early return
            ss = (d - staticVars.ad) / (d + staticVars.ad);
            q = staticVars.rr / ss;
            ss = itm_max(0.1, ss);
            q = itm_min(itm_max(0.1, q), 10.0);
            z0 = (d - staticVars.ad) * (d + staticVars.ad) * th * 0.25 / d;
            et =
                (staticVars.etq * exp(-pow(itm_min(1.7, z0 / 8.0e3), 6.0)) +
                 1.0) * z0 / 1.7556e3;
            ett = itm_max(et, 1.0);
            h0 = (h0f(r1, ett) + h0f(r2, ett)) * 0.5;
            h0 += itm_min(h0, (1.38 - log(ett)) * log(ss) * log(q) * 0.49);
            h0 = FORTRAN_DIM(h0, 0.0);
            if (et < 1.0)
                h0 =
                    et * h0 + (1.0 -
                               et) * 4.343 * log(pow((1.0 + 1.4142 / r1) *
                                                     (1.0 + 1.4142 / r2),
                                                     2.0) * (r1 + r2) / (r1 +
                                                                         r2 +
                                                                         2.8284));
            if (h0 > 15.0 && staticVars.h0s >= 0.0)
                h0 = staticVars.h0s;
        }
        staticVars.h0s = h0;
        th = propa.tha + d * prop.gme;
        ascatv =
            ahd(th * d) + 4.343 * log(47.7 * prop.wn * pow(th, 4.0)) -
            0.1 * (prop.ens - 301.0) * exp(-th * d / 40.0e3) + h0;
    }
    return ascatv;
}

// called from alos()
double abq_alos(complex < double >r) {
    return r.real() * r.real() + r.imag() * r.imag();
}

// caled from lrprop()
double alos(double d, prop_type &prop, propa_type &propa, StaticVars &staticVars) {
    complex <double> prop_zgnd(prop.zgndreal, prop.zgndimag);
    //static double wls; // static variables!
    //static StaticVars staticVars;
    complex <double> r;
    double s, sps, q;
    double alosv;
    if (d == 0.0) {
        staticVars.wls = 0.021 / (0.021 + prop.wn * prop.dh / itm_max(10.0e3, propa.dlsa));
        alosv = 0.0;
    }
    else {
        q = (1.0 - 0.8 * exp(-d / 50.0e3)) * prop.dh;
        s = 0.78 * q * exp(-pow(q / 16.0, 0.25));
        q = prop.he[0] + prop.he[1];
        sps = q / sqrt(d * d + q * q);
        r =
            (sps - prop_zgnd) / (sps +
                                 prop_zgnd) * exp(-itm_min(10.0,
                                                       prop.wn * s * sps));
        q = abq_alos(r);
        if (q < 0.25 || q < sps)
            r *= sqrt(sps / q);
        alosv = propa.emd * d + propa.aed;
        q = prop.wn * prop.he[0] * prop.he[1] * 2.0 / d;
        if (q > 1.57)
            q = 3.14 - 2.4649 / q;
        alosv =
            (-4.343 * log(abq_alos(complex < double >(cos(q), -sin(q)) + r)) -
             alosv) * staticVars.wls + alosv;
    }
    return alosv;
}

// called from itm.cpp
void qlrps(double fmhz, double zsys, double en0, __int32 ipol, double eps,
           double sgm, prop_type & prop)
{
    double gma = 157.0e-9;
    prop.wn = fmhz / 47.7;
    prop.ens = en0;
    if (zsys != 0.0)
        prop.ens *= exp(-zsys / 9460.0);
    prop.gme = gma * (1.0 - 0.04665 * exp(prop.ens / 179.3));
    complex < double >zq, prop_zgnd(prop.zgndreal, prop.zgndimag);
    zq = complex < double >(eps, 376.62 * sgm / prop.wn);
    prop_zgnd = sqrt(zq - 1.0);
    if (ipol != 0.0)
        prop_zgnd = prop_zgnd / zq;

    prop.zgndreal = prop_zgnd.real();
    prop.zgndimag = prop_zgnd.imag();
}


// called from qlrpfl() as lrprop(0.0, prop, propa)
void lrprop(double d, prop_type &prop, propa_type &propa, StaticVars &staticVars) {
    // paul_m_lrprop

    //static bool wlos, wscat; // static variables!
    //static double dmin, xae; // static variables!
    //static StaticVars staticVars;
    complex <double> prop_zgnd(prop.zgndreal, prop.zgndimag);
    double a0, a1, a2, a3, a4, a5, a6;
    double d0, d1, d2, d3, d4, d5, d6;
    bool wq;
    double q;
    int j;

    if (prop.mdp != 0) {
        for (j = 0; j < 2; j++)
            propa.dls[j] = sqrt(2.0 * prop.he[j] / prop.gme);
        propa.dlsa = propa.dls[0] + propa.dls[1];
        propa.dla = prop.dl[0] + prop.dl[1];
        propa.tha = itm_max(prop.the[0] + prop.the[1], -propa.dla * prop.gme);
        staticVars.wlos = false;
        staticVars.wscat = false;
        if (prop.wn < 0.838 || prop.wn > 210.0) {
            prop.kwx = itm_max(prop.kwx, 1);
        }
        for (j = 0; j < 2; j++)
            if (prop.hg[j] < 1.0 || prop.hg[j] > 1000.0) {
                prop.kwx = itm_max(prop.kwx, 1);
            }
        for (j = 0; j < 2; j++)
            if (abs(prop.the[j]) > 200.0e-3 || prop.dl[j] < 0.1 * propa.dls[j]
                || prop.dl[j] > 3.0 * propa.dls[j]) {   // char stemp[100];
                prop.kwx = itm_max(prop.kwx, 3);
            }
        if (prop.ens < 250.0 || prop.ens > 400.0 || prop.gme < 75.0e-9 ||
            prop.gme > 250.0e-9 || prop_zgnd.real() <= abs(prop_zgnd.imag())
            || prop.wn < 0.419 || prop.wn > 420.0) {
            prop.kwx = 4;
        }
        for (j = 0; j < 2; j++)
            if (prop.hg[j] < 0.5 || prop.hg[j] > 3000.0) {
                prop.kwx = 4;
            }
        staticVars.dmin = abs(prop.he[0] - prop.he[1]) / 200.0e-3;
        q = adiff(0.0, prop, propa, staticVars);
        staticVars.xae = pow(prop.wn * pow(prop.gme, 2.0), -THIRD);
        d3 = itm_max(propa.dlsa, 1.3787 * staticVars.xae + propa.dla);
        d4 = d3 + 2.7574 * staticVars.xae;
        a3 = adiff(d3, prop, propa, staticVars);
        a4 = adiff(d4, prop, propa, staticVars);
        propa.emd = (a4 - a3) / (d4 - d3);
        propa.aed = a3 - propa.emd * d3;
    }
    if (prop.mdp >= 0) {
        prop.mdp = 0;
        prop.dist = d;
    }
    if (prop.dist > 0.0) {
        if (prop.dist > 1000.0e3) {
            prop.kwx = itm_max(prop.kwx, 1);
        }
        if (prop.dist < staticVars.dmin) {
            prop.kwx = itm_max(prop.kwx, 3);
        }
        if (prop.dist < 1.0e3 || prop.dist > 2000.0e3) {
            prop.kwx = 4;
        }
    }
    if (prop.dist < propa.dlsa) {
        if (!staticVars.wlos) {
            q = alos(0.0, prop, propa, staticVars);
            d2 = propa.dlsa;
            a2 = propa.aed + d2 * propa.emd;
            d0 = 1.908 * prop.wn * prop.he[0] * prop.he[1];
            if (propa.aed >= 0.0) {
                d0 = itm_min(d0, 0.5 * propa.dla);
                d1 = d0 + 0.25 * (propa.dla - d0);
            }
            else
                d1 = itm_max(-propa.aed / propa.emd, 0.25 * propa.dla);
            a1 = alos(d1, prop, propa, staticVars);
            wq = false;
            if (d0 < d1) {
                a0 = alos(d0, prop, propa, staticVars);
                q = log(d2 / d0);
                propa.ak2 =
                    itm_max(0.0,
                        ((d2 - d0) * (a1 - a0) -
                         (d1 - d0) * (a2 - a0)) / ((d2 - d0) * log(d1 / d0) -
                                                   (d1 - d0) * q));
                wq = propa.aed >= 0.0 || propa.ak2 > 0.0;
                if (wq) {
                    propa.ak1 = (a2 - a0 - propa.ak2 * q) / (d2 - d0);
                    if (propa.ak1 < 0.0) {
                        propa.ak1 = 0.0;
                        propa.ak2 = FORTRAN_DIM(a2, a0) / q;
                        if (propa.ak2 == 0.0)
                            propa.ak1 = propa.emd;
                    }
                }
                else {
                    propa.ak2 = 0.0;
                    propa.ak1 = (a2 - a1) / (d2 - d1);
                    if (propa.ak1 <= 0.0)
                        propa.ak1 = propa.emd;
                }
            }
            else {
                propa.ak1 = (a2 - a1) / (d2 - d1);
                propa.ak2 = 0.0;
                if (propa.ak1 <= 0.0)
                    propa.ak1 = propa.emd;
            }
            propa.ael = a2 - propa.ak1 * d2 - propa.ak2 * log(d2);
            staticVars.wlos = true;
        }
        if (prop.dist > 0.0)
            prop.aref =
                propa.ael + propa.ak1 * prop.dist +
                propa.ak2 * log(prop.dist);
    }
    if (prop.dist <= 0.0 || prop.dist >= propa.dlsa) {
        if (!staticVars.wscat) {
            q = ascat(0.0, prop, propa, staticVars);
            d5 = propa.dla + 200.0e3;
            d6 = d5 + 200.0e3;
            a6 = ascat(d6, prop, propa, staticVars);
            a5 = ascat(d5, prop, propa, staticVars);
            if (a5 < 1000.0) {
                propa.ems = (a6 - a5) / 200.0e3;
                propa.dx =
                    itm_max(propa.dlsa,
                        itm_max(propa.dla + 0.3 * staticVars.xae * log(47.7 * prop.wn),
                            (a5 - propa.aed - propa.ems * d5) / (propa.emd -
                                                                 propa.ems)));
                propa.aes = (propa.emd - propa.ems) * propa.dx + propa.aed;
            }
            else {
                propa.ems = propa.emd;
                propa.aes = propa.aed;
                propa.dx = 10.0e6;
            }
            staticVars.wscat = true;
        }
        if (prop.dist > propa.dx)
            prop.aref = propa.aes + propa.ems * prop.dist;
        else
            prop.aref = propa.aed + propa.emd * prop.dist;
    }
    prop.aref = itm_max(prop.aref, 0.0);
}


// called from qlrpfl()
void hzns(double pfl[], prop_type & prop) {
    bool wq;
    int np;
    double xi, za, zb, qc, q, sb, sa;
    complex < double >prop_zgnd(prop.zgndreal, prop.zgndimag);

    np = RoundToInt(pfl[0]);
    xi = pfl[1];
    za = pfl[2] + prop.hg[0];
    zb = pfl[np + 2] + prop.hg[1];
    qc = 0.5 * prop.gme;
    q = qc * prop.dist;
    prop.the[1] = (zb - za) / prop.dist;
    prop.the[0] = prop.the[1] - q;
    prop.the[1] = -prop.the[1] - q;
    prop.dl[0] = prop.dist;
    prop.dl[1] = prop.dist;
    if (np >= 2) {
        sa = 0.0;
        sb = prop.dist;
        wq = true;
        for (int i = 1; i < np; ++i) {
            sa += xi;
            sb -= xi;
            q = pfl[i + 2] - (qc * sa + prop.the[0]) * sa - za;
            if (q > 0.0) {
                prop.the[0] += q / sa;
                prop.dl[0] = sa;
                wq = false;
            }
            if (!wq) {
                q = pfl[i + 2] - (qc * sb + prop.the[1]) * sb - zb;
                if (q > 0.0) {
                    prop.the[1] += q / sb;
                    prop.dl[1] = sb;
                }
            }
        }
    }
}

// called from qlrpfl()
void z1sq1(double z[], const double &x1, const double &x2, double &z0,
           double &zn)
{
    double xn, xa, xb, x, a, b;
    int n, ja, jb;
    xn = z[0];
    xa = FORTRAN_DIM(x1 / z[1], 0.0);
    xb = xn - FORTRAN_DIM(xn, x2 / z[1]);
    if (xb <= xa) {
        xa = FORTRAN_DIM(xa, 1.0);      // the 1.0 is the lowest array element
        xb = xn - FORTRAN_DIM(xn, xb + 1.0);    // should have xb not xb+1.0
    }
    ja = RoundToInt(xa);
    jb = RoundToInt(xb);
    n = jb - ja;
    xa = xb - xa;
    x = -0.5 * xa;
    xb += x;
    a = 0.5 * (z[ja + 2] + z[jb + 2]);
    b = 0.5 * (z[ja + 2] - z[jb + 2]) * x;
    if (n >= 2)
        for (int i = 2; i <= n; ++i) {
            ++ja;
            x += 1.0;
            a += z[ja + 2];
            b += z[ja + 2] * x;
        }
    a /= xa;
    b = b * 12.0 / ((xa * xa + 2.0) * xa);
    z0 = a - b * xb;
    zn = a + b * (xn - xb);
}

// called from d1thx()
double qtile(const int &nn, double a[], const int &ir) {
    double q = 0.0, r;
    int m, n, i, j, j1 = 0, i0 = 0, k;
    bool done = false;
    bool goto10 = true;

    m = 0;
    n = nn;
    k = itm_min(itm_max(0, ir), n);
    while (!done) {
        if (goto10) {
            q = a[k];
            i0 = m;
            j1 = n;
        }
        i = i0;
        while (i <= n && a[i] >= q)
            i++;
        if (i > n)
            i = n;
        j = j1;
        while (j >= m && a[j] <= q)
            j--;
        if (j < m)
            j = m;
        if (i < j) {
            r = a[i];
            a[i] = a[j];
            a[j] = r;
            i0 = i + 1;
            j1 = j - 1;
            goto10 = false;
        }
        else if (i < k) {
            a[k] = a[i];
            a[i] = q;
            m = i + 1;
            goto10 = true;
        }
        else if (j > k) {
            a[k] = a[j];
            a[j] = q;
            n = j - 1;
            goto10 = true;
        }
        else
            done = true;
    }
    return q;
}



// called from qlrpfl()
double d1thx(double pfl[], const double &x1, const double &x2) {
    int np, ka, kb, n, k, j;
    double d1thxv, sn, xa, xb;
    double *s;

    np = RoundToInt(pfl[0]);
    xa = x1 / pfl[1];
    xb = x2 / pfl[1];
    d1thxv = 0.0;
    if (xb - xa < 2.0)          // exit out
        return d1thxv;
    ka = RoundToInt(0.1 * (xb - xa + 8.0));
    ka = itm_min(itm_max(4, ka), 25);
    n = 10 * ka - 5;
    kb = n - ka + 1;
    sn = n - 1;
    assert((s = new double[n + 2])!=0);
    s[0] = sn;
    s[1] = 1.0;
    xb = (xb - xa) / sn;
    k = RoundToInt(xa + 1.0);
    xa -= (double)k;
    for (j = 0; j < n; j++) {
        while (xa > 0.0 && k < np) {
            xa -= 1.0;
            ++k;
        }
        s[j + 2] = pfl[k + 2] + (pfl[k + 2] - pfl[k + 1]) * xa;
        xa = xa + xb;
    }
    z1sq1(s, 0.0, sn, xa, xb);
    xb = (xb - xa) / sn;
    for (j = 0; j < n; j++) {
        s[j + 2] -= xa;
        xa = xa + xb;
    }
    d1thxv = qtile(n - 1, s + 2, ka - 1) - qtile(n - 1, s + 2, kb - 1);
    d1thxv /= 1.0 - 0.8 * exp(-(x2 - x1) / 50.0e3);
    delete[]s;
    return d1thxv;
}



// called from itm.cpp
void qlrpfl(double pfl[], __int32 klimx, __int32 mdvarx, prop_type & prop,
            propa_type & propa, propv_type & propv)
{
    int np, j;
    double xl[2], q, za, zb;
    StaticVars staticVars;

    prop.dist = pfl[0] * pfl[1];
    np = RoundToInt(pfl[0]);
    hzns(pfl, prop);
    for (j = 0; j < 2; ++j)
        xl[j] = itm_min(15.0 * prop.hg[j], 0.1 * prop.dl[j]);
    xl[1] = prop.dist - xl[1];
    prop.dh = d1thx(pfl, xl[0], xl[1]);
    if (prop.dl[0] + prop.dl[1] >= 1.5 * prop.dist) {
        z1sq1(pfl, xl[0], xl[1], za, zb);
        prop.he[0] = prop.hg[0] + FORTRAN_DIM(pfl[2], za);
        prop.he[1] = prop.hg[1] + FORTRAN_DIM(pfl[np + 2], zb);
        for (j = 0; j < 2; ++j)
            prop.dl[j] =
                sqrt(2.0 * prop.he[j] / prop.gme) * exp(-0.07 *
                                                        sqrt(prop.dh /
                                                             itm_max(prop.he[j],
                                                                 5.0)));
        q = prop.dl[0] + prop.dl[1];

        if (q <= prop.dist) {
            q = pow(prop.dist / q, 2.0);
            for (j = 0; j < 2; ++j) {
                prop.he[j] *= q;
                prop.dl[j] =
                    sqrt(2.0 * prop.he[j] / prop.gme) * exp(-0.07 *
                                                            sqrt(prop.dh /
                                                                 itm_max(prop.he
                                                                     [j],
                                                                     5.0)));
            }
        }
        for (j = 0; j < 2; ++j) {
            q = sqrt(2.0 * prop.he[j] / prop.gme);
            prop.the[j] =
                (0.65 * prop.dh * (q / prop.dl[j] - 1.0) -
                 2.0 * prop.he[j]) / q;
        }
    }
    else {
        z1sq1(pfl, xl[0], 0.9 * prop.dl[0], za, q);
        z1sq1(pfl, prop.dist - 0.9 * prop.dl[1], xl[1], q, zb);
        prop.he[0] = prop.hg[0] + FORTRAN_DIM(pfl[2], za);
        prop.he[1] = prop.hg[1] + FORTRAN_DIM(pfl[np + 2], zb);
    }
    prop.mdp = -1;
    propv.lvar = itm_max(propv.lvar, 3);
    if (mdvarx >= 0) {
        propv.mdvar = mdvarx;
        propv.lvar = itm_max(propv.lvar, 4);
    }
    if (klimx > 0) {
        propv.klim = klimx;
        propv.lvar = 5;
    }
    lrprop(0.0, prop, propa, staticVars);
}

#else //elseParallel

struct tcomplex
 { double tcreal;
   double tcimag;
 };

struct prop_type
{ double aref;
  double dist;
  double hg[2];
  double wn;
  double dh;
  double ens;
  double gme;
  double zgndreal;
  double zgndimag;
  double he[2];
  double dl[2];
  double the[2];
  __int32 kwx;
  __int32 mdp;
  // char *strkwx;  // Reason for kwx being off
};

struct propv_type
{ double sgc;
  __int32 lvar;
  __int32 mdvar;
  __int32 klim;
};

struct propa_type
{ double dlsa;
  double dx;
  double ael;
  double ak1;
  double ak2;
  double aed;
  double emd;
  double aes;
  double ems;
  double dls[2];
  double dla;
  double tha;
};

double FORTRAN_DIM(const double &x, const double &y)
{ // This performs the FORTRAN DIM function.
  // result is x-y if x is greater than y; otherwise result is 0.0
  if(x>y)
    return x-y;
  else
    return 0.0;
}

double aknfe(const double &v2)
{ double a;
  if(v2<5.76)
    a=6.02+9.11*sqrt(v2)-1.27*v2;
  else
    a=12.953+4.343*log(v2);
  return a;
}

double fht(const double& x, const double& pk)
{ double w, fhtv;
  if(x<200.0)
    { w=-log(pk);
          if( pk < 1.0e-5 || x*pow(w,3.0) > 5495.0 )
            { fhtv=-117.0;
                  if(x>1.0)
                    fhtv=17.372*log(x)+fhtv;
                }
          else
                fhtv=2.5e-5*x*x/pk-8.686*w-15.0;
        }
  else
        { fhtv=0.05751*x-4.343*log(x);
          if(x<2000.0)
            { w=0.0134*x*exp(-0.005*x);
                  fhtv=(1.0-w)*fhtv+w*(17.372*log(x)-117.0);
                }
        }

  return fhtv;
}

double h0f(double r, double et)
{ double a[5]={25.0, 80.0, 177.0, 395.0, 705.0};
  double b[5]={24.0, 45.0,  68.0,  80.0, 105.0};
  double q, x;
  int it;
  double h0fv;
  it=RoundToInt(et);
  if(it<=0)
    { it=1;
      q=0.0;
        }
  else if(it>=5)
    { it=5;
          q=0.0;
        }
  else
        q=et-it;
  x=pow(1.0/r,2.0);
  h0fv=4.343*log((a[it-1]*x+b[it-1])*x+1.0);
  if(q!=0.0)
    h0fv=(1.0-q)*h0fv+q*4.343*log((a[it]*x+b[it])*x+1.0);

  return h0fv;
}

double ahd(double td)
{ int i;
  double a[3] = {   133.4,    104.6,     71.8};
  double b[3] = {0.332e-3, 0.212e-3, 0.157e-3};
  double c[3] = {  -4.343,   -1.086,    2.171};
  if(td<=10.0e3)
    i=0;
  else if(td<=70.0e3)
    i=1;
  else
    i=2;

  return a[i]+b[i]*td+c[i]*log(td);
}

double  adiff( double d, prop_type &prop, propa_type &propa)
{ complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);
  static double wd1, xd1, afo, qk, aht, xht;
  double a, q, pk, ds, th, wa, ar, wd, adiffv;
  if(d==0.0)
    { q=prop.hg[0]*prop.hg[1];
          qk=prop.he[0]*prop.he[1]-q;
      if(prop.mdp<0.0)
            q+=10.0;
          wd1=sqrt(1.0+qk/q);
          xd1=propa.dla+propa.tha/prop.gme;
          q=(1.0-0.8*exp(-propa.dlsa/50.0e3))*prop.dh;
          q*=0.78*exp(-pow(q/16.0,0.25));
      afo=itm_min(15.0,2.171*log(1.0+4.77e-4*prop.hg[0]*prop.hg[1] *
                  prop.wn*q));
          qk=1.0/abs(prop_zgnd);
          aht=20.0;
          xht=0.0;
          for(int j=0;j<2;++j)
            { a=0.5*pow(prop.dl[j],2.0)/prop.he[j];
                  wa=pow(a*prop.wn,THIRD);
                  pk=qk/wa;
                  q=(1.607-pk)*151.0*wa*prop.dl[j]/a;
                  xht+=q;
                  aht+=fht(q,pk);
                }
          adiffv=0.0;
        }
  else
    { th=propa.tha+d*prop.gme;
          ds=d-propa.dla;
          q=0.0795775*prop.wn*ds*pow(th,2.0);
          adiffv=aknfe(q*prop.dl[0]/(ds+prop.dl[0]))+aknfe(q*prop.dl[1]/(ds+prop.dl[1]));
          a=ds/th;
          wa=pow(a*prop.wn,THIRD);
          pk=qk/wa;
          q=(1.607-pk)*151.0*wa*th+xht;
          ar=0.05751*q-4.343*log(q)-aht;
          q=(wd1+xd1/d)*itm_min(((1.0-0.8*exp(-d/50.0e3))*prop.dh*prop.wn),6283.2);
      wd=25.1/(25.1+sqrt(q));
          adiffv=ar*wd+(1.0-wd)*adiffv+afo;
        }

  return adiffv;
}

double  ascat( double d, prop_type &prop, propa_type &propa)
{ //complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);
  static double ad, rr, etq, h0s;
  double h0, r1, r2, z0, ss, et, ett, th, q;
  double ascatv;
  if(d==0.0)
    { ad=prop.dl[0]-prop.dl[1];
          rr=prop.he[1]/prop.he[0];
          if(ad<0.0)
            { ad=-ad;
                  rr=1.0/rr;
        }
          etq=(5.67e-6*prop.ens-2.32e-3)*prop.ens+0.031;
          h0s=-15.0;
          ascatv=0.0;
        }
  else
    { if(h0s>15.0)
            h0=h0s;
          else
            { th=prop.the[0]+prop.the[1]+d*prop.gme;
                  r2=2.0*prop.wn*th;
                  r1=r2*prop.he[0];
                  r2*=prop.he[1];
                  if(r1<0.2 && r2<0.2)
                    return 1001.0;  // <==== early return
                  ss=(d-ad)/(d+ad);
                  q=rr/ss;
                  ss=itm_max(0.1,ss);
                  q=itm_min(itm_max(0.1,q),10.0);
                  z0=(d-ad)*(d+ad)*th*0.25/d;
                  et=(etq*exp(-pow(itm_min(1.7,z0/8.0e3),6.0))+1.0)*z0/1.7556e3;
                  ett=itm_max(et,1.0);
                  h0=(h0f(r1,ett)+h0f(r2,ett))*0.5;
                  h0+=itm_min(h0,(1.38-log(ett))*log(ss)*log(q)*0.49);
                  h0=FORTRAN_DIM(h0,0.0);
                  if(et<1.0)
                    h0=et*h0+(1.0-et)*4.343*log(pow((1.0+1.4142/r1) *
                           (1.0+1.4142/r2),2.0)*(r1+r2)/(r1+r2+2.8284));
                  if(h0>15.0 && h0s>=0.0)
                    h0=h0s;
                }
      h0s=h0;
          th=propa.tha+d*prop.gme;
          ascatv=ahd(th*d)+4.343*log(47.7*prop.wn*pow(th,4.0))-0.1 *
                 (prop.ens-301.0)*exp(-th*d/40.0e3)+h0;
        }

  return ascatv;
}

double qerfi( double q )
{ double x, t, v;
  double c0  = 2.515516698;
  double c1  = 0.802853;
  double c2  = 0.010328;
  double d1  = 1.432788;
  double d2  = 0.189269;
  double d3  = 0.001308;

  x = 0.5 - q;
  t = itm_max(0.5 - fabs(x), 0.000001);
  t = sqrt(-2.0 * log(t));
  v = t - ((c2 * t + c1) * t + c0) / (((d3 * t + d2) * t + d1) * t + 1.0);
  if (x < 0.0) v = -v;

  return v;
}

double abq_alos (complex<double> r)
{ return r.real()*r.real()+r.imag()*r.imag(); }

double  alos( double d, prop_type &prop, propa_type &propa)
{ complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);
  static double wls;
  complex<double> r;
  double s, sps, q;
  double alosv;
  if(d==0.0)
    { wls=0.021/(0.021+prop.wn*prop.dh/itm_max(10.0e3,propa.dlsa));
      alosv=0.0;
        }
  else
    { q=(1.0-0.8*exp(-d/50.0e3))*prop.dh;
          s=0.78*q*exp(-pow(q/16.0,0.25));
          q=prop.he[0]+prop.he[1];
          sps=q/sqrt(d*d+q*q);
          r=(sps-prop_zgnd)/(sps+prop_zgnd)*exp(-itm_min(10.0,prop.wn*s*sps));
          q=abq_alos(r);
          if(q<0.25 || q<sps)
            r*=sqrt(sps/q);
          alosv=propa.emd*d+propa.aed;
          q=prop.wn*prop.he[0]*prop.he[1]*2.0/d;
          if(q>1.57)
            q=3.14-2.4649/q;
          alosv=(-4.343*log(abq_alos(complex<double>(cos(q),-sin(q))+r))-alosv) *
                   wls+alosv;
         }

  return alosv;
}

void qlrps( double fmhz, double zsys, double en0,
          __int32 ipol, double eps, double sgm, prop_type &prop)
{ double gma=157.0e-9;
  prop.wn=fmhz/47.7;
  prop.ens=en0;
  if(zsys!=0.0)
    prop.ens*=exp(-zsys/9460.0);
  prop.gme=gma*(1.0-0.04665*exp(prop.ens/179.3));
  complex<double> zq, prop_zgnd(prop.zgndreal,prop.zgndimag);
  zq=complex<double> (eps,376.62*sgm/prop.wn);
  prop_zgnd=sqrt(zq-1.0);
  if(ipol!=0.0)
    prop_zgnd = prop_zgnd/zq;

  prop.zgndreal=prop_zgnd.real();  prop.zgndimag=prop_zgnd.imag();
}

void qlra( __int32 kst[], __int32 klimx, __int32 mdvarx,
          prop_type &prop, propv_type &propv)
{ //complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);
  double q;
  for(int j=0;j<2;++j)
    { if(kst[j]<=0)
            prop.he[j]=prop.hg[j];
          else
            { q=4.0;
                  if(kst[j]!=1)
                    q=9.0;
                  if(prop.hg[j]<5.0)
                    q*=sin(0.3141593*prop.hg[j]);
                  prop.he[j]=prop.hg[j]+(1.0+q)*exp(-itm_min(20.0,2.0*prop.hg[j]/itm_max(1e-3,prop.dh)));
            }
          q=sqrt(2.0*prop.he[j]/prop.gme);
          prop.dl[j]=q*exp(-0.07*sqrt(prop.dh/itm_max(prop.he[j],5.0)));
          prop.the[j]=(0.65*prop.dh*(q/prop.dl[j]-1.0)-2.0*prop.he[j])/q;

        }

  prop.mdp=1;
  propv.lvar=itm_max(propv.lvar,3);
  if(mdvarx>=0)
    { propv.mdvar=mdvarx;
      propv.lvar=itm_max(propv.lvar,4);
    }
  if(klimx>0)
    { propv.klim=klimx;
          propv.lvar=5;
        }

}

void lrprop (double d,
          prop_type &prop, propa_type &propa) // paul_m_lrprop
{ static bool wlos, wscat;
  static double dmin, xae;
  complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);
  double a0, a1, a2, a3, a4, a5, a6;
  double d0, d1, d2, d3, d4, d5, d6;
  bool wq;
  double q;
  int j;

  if(prop.mdp!=0.0)
    {
          for(j=0;j<2;j++)
            propa.dls[j]=sqrt(2.0*prop.he[j]/prop.gme);
          propa.dlsa=propa.dls[0]+propa.dls[1];
          propa.dla=prop.dl[0]+prop.dl[1];
          propa.tha=itm_max(prop.the[0]+prop.the[1],-propa.dla*prop.gme);
          wlos=false;
          wscat=false;
          if(prop.wn<0.838 || prop.wn>210.0)
                { prop.kwx= itm_max(prop.kwx,1);
                }
          for(j=0;j<2;j++)
            if(prop.hg[j]<1.0 || prop.hg[j]>1000.0)
                { prop.kwx=itm_max(prop.kwx,1);
                }
          for(j=0;j<2;j++)
            if( abs(prop.the[j]) >200.0e-3 || prop.dl[j]<0.1*propa.dls[j] ||
                   prop.dl[j]>3.0*propa.dls[j] )
                { // char stemp[100];
                    prop.kwx=itm_max(prop.kwx,3);
                }
          if( prop.ens < 250.0   || prop.ens > 400.0  ||
              prop.gme < 75.0e-9 || prop.gme > 250.0e-9 ||
                  prop_zgnd.real() <= abs(prop_zgnd.imag()) ||
                  prop.wn  < 0.419   || prop.wn  > 420.0 )
                { prop.kwx=4;
                        }
          for(j=0;j<2;j++)
            if(prop.hg[j]<0.5 || prop.hg[j]>3000.0)
                { prop.kwx=4;
                }
          dmin=abs(prop.he[0]-prop.he[1])/200.0e-3;
          q=adiff(0.0,prop,propa);
          xae=pow(prop.wn*pow(prop.gme,2.0),-THIRD);
          d3=itm_max(propa.dlsa,1.3787*xae+propa.dla);
          d4=d3+2.7574*xae;
          a3=adiff(d3,prop,propa);
          a4=adiff(d4,prop,propa);
          propa.emd=(a4-a3)/(d4-d3);
          propa.aed=a3-propa.emd*d3;
     }
  if(prop.mdp>=0.0)
    {   prop.mdp=0;
        prop.dist=d;
    }
  if(prop.dist>0.0)
    {
          if(prop.dist>1000.0e3)
            { prop.kwx=itm_max(prop.kwx,1);
            }
          if(prop.dist<dmin)
            { prop.kwx=itm_max(prop.kwx,3);
            }
          if(prop.dist<1.0e3 || prop.dist>2000.0e3)
            { prop.kwx=4;
            }
    }
  if(prop.dist<propa.dlsa)
    {
          if(!wlos)
            {
                q=alos(0.0,prop,propa);
                d2=propa.dlsa;
                a2=propa.aed+d2*propa.emd;
                d0=1.908*prop.wn*prop.he[0]*prop.he[1];
                if(propa.aed>=0.0)
                    { d0=itm_min(d0,0.5*propa.dla);
                      d1=d0+0.25*(propa.dla-d0);
                    }
                else
                      d1=itm_max(-propa.aed/propa.emd,0.25*propa.dla);
                a1=alos(d1,prop,propa);
                wq=false;
                if(d0<d1)
                    {
                        a0=alos(d0,prop,propa);
                        q=log(d2/d0);
                        propa.ak2=itm_max(0.0,((d2-d0)*(a1-a0)-(d1-d0)*(a2-a0)) /
                                        ((d2-d0)*log(d1/d0)-(d1-d0)*q));
                        wq=propa.aed>=0.0 || propa.ak2>0.0;
                        if(wq)
                        {
                                propa.ak1=(a2-a0-propa.ak2*q)/(d2-d0);
                                if(propa.ak1<0.0)
                                { propa.ak1=0.0;
                                  propa.ak2=FORTRAN_DIM(a2,a0)/q;
                                  if(propa.ak2==0.0)
                                        propa.ak1=propa.emd;
                                }
                        }
                        else
                        {
                                propa.ak2=0.0;
                                propa.ak1=(a2-a1)/(d2-d1);
                                if(propa.ak1<=0.0)
                                        propa.ak1=propa.emd;
                        }
                    }
              else
                    {   propa.ak1=(a2-a1)/(d2-d1);
                        propa.ak2=0.0;
                        if(propa.ak1<=0.0)
                                propa.ak1=propa.emd;
                    }

              propa.ael=a2-propa.ak1*d2-propa.ak2*log(d2);
              wlos=true;
            }
      if(prop.dist>0.0)
        prop.aref=propa.ael+propa.ak1*prop.dist +
                       propa.ak2*log(prop.dist);

    }
  if(prop.dist<=0.0 || prop.dist>=propa.dlsa)
    { if(!wscat)
            {
                  q=ascat(0.0,prop,propa);
                  d5=propa.dla+200.0e3;
                  d6=d5+200.0e3;
                  a6=ascat(d6,prop,propa);
                  a5=ascat(d5,prop,propa);
                  if(a5<1000.0)
                    { propa.ems=(a6-a5)/200.0e3;
                      propa.dx=itm_max(propa.dlsa,itm_max(propa.dla+0.3*xae *
                                 log(47.7*prop.wn),(a5-propa.aed-propa.ems*d5) /
                                         (propa.emd-propa.ems)));
                      propa.aes=(propa.emd-propa.ems)*propa.dx+propa.aed;
                    }
                  else
                    { propa.ems=propa.emd;
                      propa.aes=propa.aed;
                      propa.dx=10.0e6;
                    }
                  wscat=true;
            }
          if(prop.dist>propa.dx){
            prop.aref=propa.aes+propa.ems*prop.dist;

         } else{
            prop.aref=propa.aed+propa.emd*prop.dist;

}
    }
  prop.aref=itm_max(prop.aref,0.0);
}


void freds_lrprop (double d,
          prop_type &prop, propa_type &propa) // freds_lrprop
{ static bool wlos, wscat;
  static double dmin, xae;
  complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);
  double a0, a1, a2, a3, a4, a5, a6;
  double d0, d1, d2, d3, d4, d5, d6;
  double q;
  int j;

   if(prop.mdp!=0)
    {
          for(j=0;j<2;++j)
            propa.dls[j]=sqrt(2.0*prop.he[j]/prop.gme);
          propa.dlsa=propa.dls[0]+propa.dls[1];
          propa.dla=prop.dl[0]+prop.dl[1];
          propa.tha=itm_max(prop.the[0]+prop.the[1],-propa.dla*prop.gme);
      wlos=false;
          wscat=false;
          if(prop.wn<0.838 || prop.wn>210.0)
        { prop.kwx=itm_max(prop.kwx,1);
                }
          for(j=0;j<2;++j)
            if(prop.hg[j]<1.0 || prop.hg[j]>1000.0)
          { prop.kwx=itm_max(prop.kwx,1);
                  }
          for(j=0;j<2;++j)
            if( abs(prop.the[j]) >200.0e-3 || prop.dl[j]<0.1*propa.dls[j] ||
                   prop.dl[j]>3.0*propa.dls[j] )
          { prop.kwx=itm_max(prop.kwx,3);
                  }
          if( prop.ens < 250.0   || prop.ens > 400.0  ||
              prop.gme < 75.0e-9 || prop.gme > 250.0e-9 ||
                  prop_zgnd.real() <= abs(prop_zgnd.imag()) ||
                  prop.wn  < 0.419   || prop.wn  > 420.0 )
             { prop.kwx=4;
                 }
      for(j=0;j<2;++j)
            if(prop.hg[j]<0.5 || prop.hg[j]>3000.0)
                  { prop.kwx=4;
                  }
          dmin=abs(prop.he[0]-prop.he[1])/200.0e-3;
          q=adiff(0.0,prop,propa);
          xae=pow(prop.wn * prop.gme * prop.gme,-THIRD);
          d3=itm_max(propa.dlsa,1.3787 * xae+propa.dla);
          d4=d3+2.7574 * xae;
          a3=adiff(d3,prop,propa);
          a4=adiff(d4,prop,propa);
      propa.emd=(a4-a3)/(d4-d3);
          propa.aed=a3-propa.emd * d3;
      if(prop.mdp==0.0)
            prop.dist=0.0;
          else if(prop.mdp>0.0)
            { prop.mdp=0;  prop.dist=0.0; }
          if((prop.dist>0.0) || (prop.mdp<0.0))
        {
              if(prop.dist>1000.0e3)
                { prop.kwx=itm_max(prop.kwx,1);
                    }
              if(prop.dist<dmin)
                { prop.kwx=itm_max(prop.kwx,3);
                    }
              if(prop.dist<1.0e3 || prop.dist>2000.0e3)
                { prop.kwx=4;
                    }
            }
        }
  else
    { prop.dist=d;
      if(prop.dist>0.0)
        {
              if(prop.dist>1000.0e3)
                { prop.kwx=itm_max(prop.kwx,1);
                    }
              if(prop.dist<dmin)
                { prop.kwx=itm_max(prop.kwx,3);
                    }
              if(prop.dist<1.0e3 || prop.dist>2000.0e3)
                { prop.kwx=4;
                    }
            }
        }

  if(prop.dist<propa.dlsa)
    {
      if(!wlos)
            {
          // Cooeficients for the line-of-sight range

          q=alos(0.0,prop,propa);
          d2=propa.dlsa;
          a2=propa.aed+d2*propa.emd;
          d0=1.908*prop.wn*prop.he[0]*prop.he[1];
          if(propa.aed>=0.0)
            { d0=itm_min(d0,0.5*propa.dla);
                  d1=d0+0.25*(propa.dla-d0);
                }
          else
            d1=itm_max(-propa.aed/propa.emd,0.25*propa.dla);
          a1=alos(d1,prop,propa);

          if(d0<d1)
            { a0=alos(d0,prop,propa);
                  q=log(d2/d0);
                  propa.ak2=itm_max(0.0,((d2-d0)*(a1-a0)-(d1-d0)*(a2-a0)) /
                            ((d2-d0)*log(d1-d0)-(d1-d0)*q));
                  if(propa.ak2<=0.0 && propa.aed<0.0)
                    { propa.ak2=0.0;
                          propa.ak1=(a2-a1)/(d2-d1);
                          if(propa.ak1<=0.0)
                            propa.ak2=propa.emd;
                        }
                  else
                    { propa.ak1=(a2-a0-propa.ak2*q)/(d2-d0);
                          if(propa.ak1<0.0)
                            { propa.ak1=0.0;
                                  propa.ak2=FORTRAN_DIM(a2,a0)/q;
                                  if(propa.ak2<=0.0)
                                    propa.ak1=propa.emd;
                                }
                        }
                  propa.ael=a2-propa.ak1*d2-propa.ak2*log(d2);
                  wlos=true;
                }
            }
      if(prop.dist>0.0)
            prop.aref=propa.ael+propa.ak1*prop.dist+propa.ak2*log(prop.dist);
        }
  else
    { if(!wscat)
            { q=ascat(0.0,prop,propa);
                  d5=propa.dla+200.0e3;
                  d6=d5+200.0e3;
                  a6=ascat(d6,prop,propa);
                  a5=ascat(d5,prop,propa);
                  if(a5>=1000.0)
                    { propa.ems=propa.emd;
                          propa.aes=propa.aed;
                          propa.dx=10.0e6;
                        }
                  else
                    { propa.ems=(a6-a5)/200.0e3;
              propa.dx=itm_max(propa.dlsa,itm_max(propa.dla+0.3*xae*log(47.7*prop.wn),
                                   (a5-propa.aed-propa.ems*d5)/(propa.emd-propa.ems)));
                          propa.aes=(propa.emd-propa.ems)*propa.dx+propa.aed;
                        }
                  wscat=true;
                }
          if(prop.dist<=propa.dx)
            prop.aref=propa.aed+propa.emd*prop.dist;
          else
            prop.aref=propa.aes+propa.ems*prop.dist;
        }
  prop.aref=FORTRAN_DIM(prop.aref,0.0);

}


double curve (double const &c1, double const &c2, double const &x1,
              double const &x2, double const &x3, double const &de)
{ return (c1+c2/(1.0+pow((de-x2)/x3,2.0)))*pow(de/x1,2.0) /
         (1.0+pow(de/x1,2.0));
}

double avar (double zzt, double zzl, double zzc,
         prop_type &prop, propv_type &propv)
{ static int kdv;
  static double dexa, de, vmd, vs0, sgl, sgtm, sgtp, sgtd, tgtd,
                gm, gp, cv1, cv2, yv1, yv2, yv3, csm1, csm2, ysm1, ysm2,
                                ysm3, csp1, csp2, ysp1, ysp2, ysp3, csd1, zd, cfm1, cfm2,
                                cfm3, cfp1, cfp2, cfp3;
  double bv1[7]={-9.67,-0.62,1.26,-9.21,-0.62,-0.39,3.15};
  double bv2[7]={12.7,9.19,15.5,9.05,9.19,2.86,857.9};
  double xv1[7]={144.9e3,228.9e3,262.6e3,84.1e3,228.9e3,141.7e3,2222.e3};
  double xv2[7]={190.3e3,205.2e3,185.2e3,101.1e3,205.2e3,315.9e3,164.8e3};
  double xv3[7]={133.8e3,143.6e3,99.8e3,98.6e3,143.6e3,167.4e3,116.3e3};
  double bsm1[7]={2.13,2.66,6.11,1.98,2.68,6.86,8.51};
  double bsm2[7]={159.5,7.67,6.65,13.11,7.16,10.38,169.8};
  double xsm1[7]={762.2e3,100.4e3,138.2e3,139.1e3,93.7e3,187.8e3,609.8e3};
  double xsm2[7]={123.6e3,172.5e3,242.2e3,132.7e3,186.8e3,169.6e3,119.9e3};
  double xsm3[7]={94.5e3,136.4e3,178.6e3,193.5e3,133.5e3,108.9e3,106.6e3};
  double bsp1[7]={2.11,6.87,10.08,3.68,4.75,8.58,8.43};
  double bsp2[7]={102.3,15.53,9.60,159.3,8.12,13.97,8.19};
  double xsp1[7]={636.9e3,138.7e3,165.3e3,464.4e3,93.2e3,216.0e3,136.2e3};
  double xsp2[7]={134.8e3,143.7e3,225.7e3,93.1e3,135.9e3,152.0e3,188.5e3};
  double xsp3[7]={95.6e3,98.6e3,129.7e3,94.2e3,113.4e3,122.7e3,122.9e3};
  double bsd1[7]={1.224,0.801,1.380,1.000,1.224,1.518,1.518};
  double bzd1[7]={1.282,2.161,1.282,20.,1.282,1.282,1.282};
  double bfm1[7]={1.0,1.0,1.0,1.0,0.92,1.0,1.0};
  double bfm2[7]={0.0,0.0,0.0,0.0,0.25,0.0,0.0};
  double bfm3[7]={0.0,0.0,0.0,0.0,1.77,0.0,0.0};
  double bfp1[7]={1.0,0.93,1.0,0.93,0.93,1.0,1.0};
  double bfp2[7]={0.0,0.31,0.0,0.19,0.31,0.0,0.0};
  double bfp3[7]={0.0,2.00,0.0,1.79,2.00,0.0,0.0};
  static bool ws, w1;
  double rt=7.8, rl=24.0, avarv, q, vs, zt, zl, zc;
  double sgt, yr;
  int temp_klim = propv.klim-1;

  if(propv.lvar>0.0)
    { if(propv.lvar<=5.0)
        switch(propv.lvar)
             { default:
                     if(propv.klim<=0.0 || propv.klim>7.0)
                           { propv.klim = 5;
                             temp_klim = 4;
                             { prop.kwx=itm_max(prop.kwx,2);
                                 }
                           }
                         cv1 = bv1[temp_klim];
             cv2 = bv2[temp_klim];
                         yv1 = xv1[temp_klim];
                         yv2 = xv2[temp_klim];
                         yv3 = xv3[temp_klim];
                         csm1=bsm1[temp_klim];
                         csm2=bsm2[temp_klim];
                         ysm1=xsm1[temp_klim];
                         ysm2=xsm2[temp_klim];
                         ysm3=xsm3[temp_klim];
                         csp1=bsp1[temp_klim];
                         csp2=bsp2[temp_klim];
                         ysp1=xsp1[temp_klim];
                         ysp2=xsp2[temp_klim];
                         ysp3=xsp3[temp_klim];
                         csd1=bsd1[temp_klim];
                         zd  =bzd1[temp_klim];
                         cfm1=bfm1[temp_klim];
                         cfm2=bfm2[temp_klim];
                         cfm3=bfm3[temp_klim];
                         cfp1=bfp1[temp_klim];
                         cfp2=bfp2[temp_klim];
                         cfp3=bfp3[temp_klim];
                   case 4:
             kdv=propv.mdvar;
                         ws = kdv>=20.0;
             if(ws)
                           kdv-=20;
                         w1 = kdv>=10.0;
                         if(w1)
                           kdv-=10;
                         if(kdv<0.0 || kdv>3.0)
                           { kdv=0;
                             prop.kwx=itm_max(prop.kwx,2);
                           }
                   case 3:
                     q=log(0.133*prop.wn);
                         gm=cfm1+cfm2/(pow(cfm3*q,2.0)+1.0);
                         gp=cfp1+cfp2/(pow(cfp3*q,2.0)+1.0);
                   case 2:
                     dexa=sqrt(18e6*prop.he[0])+sqrt(18e6*prop.he[1]) +
                              pow((575.7e12/prop.wn),THIRD);
                   case 1:
                     if(prop.dist<dexa)
                           de=130.0e3*prop.dist/dexa;
                         else
                           de=130.0e3+prop.dist-dexa;
                }
        vmd=curve(cv1,cv2,yv1,yv2,yv3,de);
                sgtm=curve(csm1,csm2,ysm1,ysm2,ysm3,de) * gm;
                sgtp=curve(csp1,csp2,ysp1,ysp2,ysp3,de) * gp;
                sgtd=sgtp*csd1;
                tgtd=(sgtp-sgtd)*zd;
                if(w1)
                  sgl=0.0;
                else
                  { q=(1.0-0.8*exp(-prop.dist/50.0e3))*prop.dh*prop.wn;
                    sgl=10.0*q/(q+13.0);
                  }
                if(ws)
                  vs0=0.0;
                else
                  vs0=pow(5.0+3.0*exp(-de/100.0e3),2.0);
                propv.lvar=0;
        }
  zt=zzt;
  zl=zzl;
  zc=zzc;
  switch(kdv)
    { case 0:
        zt=zc;
            zl=zc;
            break;
      case 1:
        zl=zc;
        break;
          case 2:
            zl=zt;
        }
  if(fabs(zt)>3.1 || fabs(zl)>3.1 || fabs(zc)>3.1)
      { prop.kwx=itm_max(prop.kwx,1);
          }
  if(zt<0.0)
    sgt=sgtm;
  else if(zt<=zd)
    sgt=sgtp;
  else
    sgt=sgtd+tgtd/zt;
  vs=vs0+pow(sgt*zt,2.0)/(rt+zc*zc)+pow(sgl*zl,2.0)/(rl+zc*zc);
  if(kdv==0)
    { yr=0.0;
      propv.sgc=sqrt(sgt*sgt+sgl*sgl+vs);
    }
  else if(kdv==1)
    { yr=sgt*zt;
      propv.sgc=sqrt(sgl*sgl+vs);
    }
  else if(kdv==2)
    { yr=sqrt(sgt*sgt+sgl*sgl)*zt;
      propv.sgc=sqrt(vs);
    }
  else
    { yr=sgt*zt+sgl*zl;
      propv.sgc=sqrt(vs);
    }
  avarv=prop.aref-vmd-yr-propv.sgc*zc;
  if(avarv<0.0)
    avarv=avarv*(29.0-avarv)/(29.0-10.0*avarv);

  return avarv;

}

void hzns (double pfl[], prop_type &prop)
{ bool wq;
  int np;
  double xi, za, zb, qc, q, sb, sa;
  //complex<double> prop_zgnd(prop.zgndreal,prop.zgndimag);

  np=RoundToInt(pfl[0]);
  xi=pfl[1];
  za=pfl[2]+prop.hg[0];
  zb=pfl[np+2]+prop.hg[1];
  qc=0.5*prop.gme;
  q=qc*prop.dist;
  prop.the[1]=(zb-za)/prop.dist;
  prop.the[0]=prop.the[1]-q;
  prop.the[1]=-prop.the[1]-q;
  prop.dl[0]=prop.dist;
  prop.dl[1]=prop.dist;
  if(np>=2)
    { sa=0.0;
          sb=prop.dist;
          wq=true;
          for(int i=1;i<np;++i)
            { sa+=xi;
                  sb-=xi;
                  q=pfl[i+2]-(qc*sa+prop.the[0])*sa-za;
                  if(q>0.0)
                    { prop.the[0]+=q/sa;
                          prop.dl[0]=sa;
                          wq=false;
                        }
                  if(!wq)
                    { q=pfl[i+2]-(qc*sb+prop.the[1])*sb-zb;
                      if(q>0.0)
                            { prop.the[1]+=q/sb;
                                  prop.dl[1]=sb;
                                }
                        }
                }
        }
}



void z1sq1 (double z[], const double &x1, const double &x2,
            double& z0, double& zn)
{ double xn, xa, xb, x, a, b;
  int n, ja, jb;
  xn=z[0];

  xa= FORTRAN_DIM(x1/z[1],0.0);
  xb=xn-FORTRAN_DIM(xn,x2/z[1]);

  if(xb<=xa)
      { xa=FORTRAN_DIM(xa,1.0);  // the 1.0 is the lowest array element
            xb=xn-FORTRAN_DIM(xn,xb+1.0); // should have xb not xb+1.0
        }
  ja=RoundToInt(xa);
  jb=RoundToInt(xb);
  n=jb-ja;
  xa=xb-xa;
  x=-0.5*xa;
  xb+=x;
  a=0.5*(z[ja+2]+z[jb+2]);
  b=0.5*(z[ja+2]-z[jb+2])*x;
  if(n>=2)
    for(int i=2;i<=n;++i)
      { ++ja;
            x+=1.0;
            a+=z[ja+2];
            b+=z[ja+2]*x;
          }
  a/=xa;
  b=b*12.0/((xa*xa+2.0)*xa);
  z0=a-b*xb;
  zn=a+b*(xn-xb);

}

double qtile (const int &nn, double a[], const int &ir)
{ double q = 0.0, r;
  int m, n, i, j, j1 = 0, i0 = 0, k;
  bool done=false;
  bool goto10=true;

  m=0;
  n=nn;
  k=itm_min(itm_max(0,ir),n);
  while(!done)
      {
      if(goto10)
        {  q=a[k];
          i0=m;
          j1=n;
        }
      i=i0;
      while(i<=n && a[i]>=q)
            i++;
      if(i>n)
            i=n;
      j=j1;
      while(j>=m && a[j]<=q)
            j--;
      if(j<m)
            j=m;
      if(i<j)
            {     r=a[i]; a[i]=a[j]; a[j]=r;
                  i0=i+1;
                  j1=j-1;
                  goto10=false;
            }
      else if(i<k)
            {     a[k]=a[i];
                  a[i]=q;
                  m=i+1;
                  goto10=true;
            }
      else if(j>k)
            {     a[k]=a[j];
                  a[j]=q;
                  n=j-1;
                  goto10=true;
            }
      else
            done=true;
      }

  return q;
}

double myqtile (const int &nn, double a[], const int &ir)
{ double q, r;
  int m, n, i, j, j1, i0, k;
  bool done=false;

  m=1;
  n=nn;
  k=itm_min(itm_max(1,ir),n);
  q=a[k-1];
  i0=m;
  j1=n;
  while(!done)
    { i=i0;
      while(i<=n && a[i-1]>=q)
            ++i;
          if(i>n)
            i=n;
      j=j1;
          while(j>=m && a[j-1]<=q)
            --j;
          if(j<m)
            j=m;
          if(i<j)
            { r=a[i-1]; a[i-1]=a[j-1]; a[j-1]=r;
                  i0=i+1;
                  j1=j-1;
                }
          else if(i<k)
            { a[k-1]=a[i-1];
                  a[i-1]=q;
                  m=i+1;
          q=a[k-1];
          i0=m;
          j1=n;
        }
          else
            {  if(j>k)
                 { a[k-1]=a[j-1];
                       a[j-1]=q;
                       n=j-1;
                       q=a[k-1];
               i0=m;
                   j1=n;
             }
               else
                 done=true;
                }
        } // while(!done)

  return q;
}

double qerf(const double &z)
{ double b1=0.319381530, b2=-0.356563782, b3=1.781477937;
  double b4=-1.821255987, b5=1.330274429;
  double rp=4.317008, rrt2pi=0.398942280;
  double t, x, qerfv;
  x=z;
  t=fabs(x);
  if(t>=10.0)
    qerfv=0.0;
  else
    { t=rp/(t+rp);
          qerfv=exp(-0.5*x*x)*rrt2pi*((((b5*t+b4)*t+b3)*t+b2)*t+b1)*t;
        }
  if(x<0.0) qerfv=1.0-qerfv;

  return qerfv;
}

double d1thx(double pfl[], const double &x1, const double &x2)
{ int np, ka, kb, n, k, j;
  double d1thxv, sn, xa, xb;
  double *s;

  np=RoundToInt(pfl[0]);
  xa=x1/pfl[1];
  xb=x2/pfl[1];
  d1thxv=0.0;
  if((xb-xa)<2.0)  // exit out
    return d1thxv;
  ka= RoundToInt(0.1*(xb-xa+8.0));
  ka=itm_min(itm_max(4,ka),25);
  n=10*ka-5;
  kb=n-ka+1;
  sn=n-1.0;
  assert( (s = new double[n+2]) != 0 );
  s[0]=sn;
  s[1]=1.0;
  xb=(xb-xa)/sn;
  k=RoundToInt(xa+1.0);
  xa-=(double)k;
  for(j=0;j<n;j++)
    { while(xa>0.0 && k<np)
            { xa-=1.0;
                  ++k;
                }
          s[j+2]=pfl[k+2]+(pfl[k+2]-pfl[k+1])*xa;
          xa=xa+xb;
        }
  z1sq1(s,0.0,sn,xa,xb);
  xb=(xb-xa)/sn;
  for(j=0;j<n;j++)
    { s[j+2]-=xa;
          xa=xa+xb;
        }
  d1thxv=qtile(n-1,s+2,ka-1)-qtile(n-1,s+2,kb-1);
  d1thxv/=1.0-0.8*exp(-(x2-x1)/50.0e3);
  delete[] s;

  return d1thxv;
}


double myd1thx(double pfl[], const double &x1, const double &x2)
{ int np, ka, kb, n, k, j;
  double d1thxv, sn, xa, xb;
  double *s;

  np=RoundToInt(pfl[0]);
  xa=x1/pfl[1];
  xb=x2/pfl[1];
  d1thxv=0.0;
  if(xb-xa<2.0)  // exit out
    return d1thxv;
  ka=RoundToInt(0.1*(xb-xa+8.0));
  ka=itm_min(itm_max(4,ka),25);
  n=10*ka-5;
  kb=n-ka+1;
  sn=n-1.0;
  assert( (s = new double[n+3]) != 0 );
  s[0]=sn;
  s[1]=1.0;
  xb=(xb-xa)/sn;
  k=RoundToInt(xa+1.0);
  xa-=(double)k;
  for(j=0;j<n;++j)  // changed from for(j=0;j<n;++j)
    { while(xa>0.0 && k<np)
            { xa-=1.0;
                  ++k;
                }
          s[j+2]=pfl[k+2]+(pfl[k+2]-pfl[k+1])*xa;
          xa+=xb;
        }
  z1sq1(s,0.0,sn,xa,xb);
  xb=(xb-xa)/sn;
  for(j=0;j<n;++j)   // j=0;j<n-1;j++
                       // changed from for(j=0;j<n;++j)
    { s[j+2]-=xa;
          xa+=xb;
        }
  d1thxv=qtile(n,s+2,ka)-qtile(n,s+2,kb);
  d1thxv/=1.0-0.8*exp(-(x2-x1)/50.0e3);
  delete[] s;

  return d1thxv;
}

void qlrpfl( double pfl[], __int32 klimx, __int32 mdvarx,
        prop_type &prop, propa_type &propa, propv_type &propv )
{ int np, j;
  double xl[2], q, za, zb;

  prop.dist=pfl[0]*pfl[1];
  np=RoundToInt(pfl[0]);
  hzns(pfl,prop);
  for(j=0;j<2;++j)
    xl[j]=itm_min(15.0*prop.hg[j],0.1*prop.dl[j]);
  xl[1]=prop.dist-xl[1];
  prop.dh=d1thx(pfl,xl[0],xl[1]);
  if(prop.dl[0]+prop.dl[1]>=1.5*prop.dist)
    { z1sq1(pfl,xl[0],xl[1],za,zb);
          prop.he[0]=prop.hg[0]+FORTRAN_DIM(pfl[2],za);
          prop.he[1]=prop.hg[1]+FORTRAN_DIM(pfl[np+2],zb);
      for(j=0;j<2;++j)
            prop.dl[j]=sqrt(2.0*prop.he[j]/prop.gme) *
                            exp(-0.07*sqrt(prop.dh/itm_max(prop.he[j],5.0)));
      q=prop.dl[0]+prop.dl[1];

      if(q<=prop.dist)
            { q=pow(prop.dist/q,2.0);
                  for(j=0;j<2;++j)
            { prop.he[j]*=q;
                          prop.dl[j]=sqrt(2.0*prop.he[j]/prop.gme) *
                            exp(-0.07*sqrt(prop.dh/itm_max(prop.he[j],5.0)));
                        }
                }
          for(j=0;j<2;++j)
            { q=sqrt(2.0*prop.he[j]/prop.gme);
                  prop.the[j]=(0.65*prop.dh*(q/prop.dl[j]-1.0)-2.0 *
                               prop.he[j])/q;
        }
        }
  else
    { z1sq1(pfl,xl[0],0.9*prop.dl[0],za,q);
          z1sq1(pfl,prop.dist-0.9*prop.dl[1],xl[1],q,zb);
          prop.he[0]=prop.hg[0]+FORTRAN_DIM(pfl[2],za);
          prop.he[1]=prop.hg[1]+FORTRAN_DIM(pfl[np+2],zb);
        }
  prop.mdp=-1;
  propv.lvar=itm_max(propv.lvar,3);
  if(mdvarx>=0)
    { propv.mdvar=mdvarx;
      propv.lvar=itm_max(propv.lvar,4);
    }
  if(klimx>0)
    { propv.klim=klimx;
          propv.lvar=5;
        }
  lrprop(0.0,prop,propa);
}

#endif //endParallel
#endif
