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
//
#ifndef __ANE_ANESAT_TC_H__
# define __ANE_ANESAT_TC_H__

///
/// Uncomment ANESAT_DEBUG_TC to debug the traffic conditioning code
///
//#define ANESAT_DEBUG_TC 


/// @brief Base class for all traffic conditioners
///
/// @Class AneSatTrafficConditioner
///
/// Base class contains the virtual members to support all conditioners
/// <ul>
/// <li> getBandwidthAvailable
/// <li> setBandwidthLimit
/// </ul>
///
/// This class also contains a basic long-term bandwidth utilization estate that 
/// essentially calculates \f$ r_{offered} = \frac{l_{sent}}{t_{sim}} \f$
///
class AneSatTrafficConditioner {
protected:
    double bitsSent; //!< total bits sent count since epoch (or reset)
public:
    ///
    /// Base class initializer/destructor
    /// 
    AneSatTrafficConditioner() : bitsSent(0.0) { }
    ~AneSatTrafficConditioner() { }
    /// @brief tell class that bits have been sent out on conditioner
    /// 
    /// @param bits bits sent by interface
    ///
    void reportBitsSent(int bits) {
        bitsSent += (double)bits;
    }
    /// @brief reset bits sent to zero
    ///
    void reset(void) { 
        bitsSent = 0.0;
    }
    /// @brief virtual member for all subordinate classes gets bandwidth available
    ///
    /// @returns bandwidth available (bits/sec)
    /// @param node is the node running the conditioner
    ///
    virtual double getBandwidthAvailable(Node *node) = 0;
    /// @brief virtual member for all subordinate classes set the local conditioner limits
    ///
    /// @param updatedLimit is the new limit in bits/sec
    ///
    /// Default action is to _reject_ all attempts to set the limit and end the simulation (programming error).
    virtual void setBandwidthLimit(double updatedLimit) {
        ERROR_ReportError("Cannot send bandwidth limit dynamically in this type of conditioner, must be STRICT.");
    }
} ;

/// @brief Strict traffic conditioner
///
/// @class AneSatStrictTrafficConditioner
///
/// This class extends the basic traffic conditioner to do strict conditioning
/// The idea of strict conditioning is that the conditioner will always return the same
/// limit set by the programmer (dynamically) or user (via the config file).
///
/// The bandwidth limit can be set via the node API @see MAC_ManagementRequest()
///
class AneSatStrictTrafficConditioner : public AneSatTrafficConditioner {
    double bwLimit; //!< The current bandwidth limit in bits/sec
public:
    AneSatStrictTrafficConditioner(double pBwLimit) : bwLimit(pBwLimit) { }
    ~AneSatStrictTrafficConditioner() { }
    /// @brief return bandwidth available (bits/sec)
    ///
    /// @param node the node data structure which is imposing the limit
    /// @returns the bandwidth available after conditioning in bis/sec
    ///
    double getBandwidthAvailable(Node *node) { return bwLimit; }
    /// @brief set bandwidth limit (in bits/sec)
    ///
    /// @param updatedLimit new/proposed bandwidth limit in bits/sec
    /// 
    void setBandwidthLimit(double updatedLimit) { 
#if defined(DEBUG_PR4785)
        printf("Updating bandwidth limit to %0.3e from %0.3e.\n", updatedLimit, bwLimit);
#endif
        bwLimit = updatedLimit;
    }
} ;

/// @brief Residual traffic conditioner class
///
/// @class AneSatResidualTrafficConditioner
///
/// This traffic conditioner is a residual traffic conditioner and allocates traffic based on limits and
/// historical bandwidth utilization.
///
/// The calculation is \f$r_{res} = \frac{l_{sent}}{t_{sim}} \f$.  The overall remaining bandwidth is calculated as
/// \f$r_{avail} = 2 r_{limit} - r_{res} \f$
/// Finally the available bandwidth cannot fall below the minimum bandwidth \f$r_{min}\f$ so the routines returns
/// \f${\rm max}(r_{min},r_{avail})\f$
///
class AneSatResidualTrafficConditioner : public AneSatTrafficConditioner {
protected:
    double bwLimit, minBw;
    /// @brief Get residual bandwidth from filter
    ///
    /// @param node Node data structure that hosts the traffic conditioner
    /// @returns The residual bandwidth in bits/sec
    ///
    double getResidualUsage(Node *node) {
        double now = (double)node->getNodeTime() / (double)SECOND;
        if (now > 0.0) {
            return bitsSent / now;
        }
        return 0.0;
    }
public:
    AneSatResidualTrafficConditioner(double pBwLimit, double pBwMinimum) : bwLimit(pBwLimit), minBw(pBwMinimum) { }
    ~AneSatResidualTrafficConditioner() { }
    /// @brief Calculate available bandwidth
    ///
    /// @param node Node data structure that hosts the traffic conditioner
    /// @returns The bandwidth available from the conditioner
    ///
    /// This routine implements the calculations above @see class AneSatResidualTrafficConditioner
    ///
    double getBandwidthAvailable(Node *node) {
        double residue = getResidualUsage(node);
        double remainder = 2.0 * bwLimit - residue;
        
        if (remainder < minBw) {
            remainder = minBw;
        }

#ifdef ANESAT_DEBUG_TC
    printf("-->conditioner { rmin=%0.3e, rres=%0.3e, rrem=%0.3e }\n", minBw, residue, remainder);
#endif // ANESAT_DEBUG_TC
        
        return remainder;
    }
} ;

//
// Header declarations
//
//

#endif                          /* __ANE_ANESAT_DEFAULT_MAC_H__ */
