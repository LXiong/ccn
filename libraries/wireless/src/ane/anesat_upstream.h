/// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
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
#ifndef __ANE_ANESAT_UPSTREAM_H__
# define __ANE_ANESAT_UPSTREAM_H__

/// @brief Per-upstream data structure (may be shared)
///
/// @Class AneSatUpstreamData
/// 
/// This data structure is created to track upstream data 
/// utilizaton and scheduling state.
/// In particular it tracks:
/// <ul>
/// <li> First Available Transmit Time: when the upstream is next 
/// free (absolute time)
/// <li> Bandwidth of the upstream channel: the serialization rate of 
/// the channel (bits/sec)
/// <li> Access Latency: the time to access the channel (sec)
/// </ul>
///

class AneSatUpstreamData 
{
protected:
    
    double firstAvailableTime;
    double bandwidth; 
    double accessLatency;
   
public:

 
    AneSatUpstreamData() 
    : firstAvailableTime(0.0), bandwidth(0.0), accessLatency(0.0) 
    { 
    
    }
    
    ~AneSatUpstreamData() 
    { 
    
    }
    
    /// @brief Set bandwidth of channel
    /// 
    /// @param bw New bandwidth of channel (bits/sec)
    ///
    
    void setBandwidth(double bw) 
    { 
        bandwidth = bw;
    }
    
    /// @brief Get bandwidth of channel
    ///
    /// @returns Bandwidth of channel (bits/sec)
    ///
    
    double getBandwidth(void) 
    { 
        return bandwidth; 
    }
    
    /// @brief Set base (static) access latency
    ///
    /// @param lat The new access latency (in seconds)
    ///
    
    void setAccessLatencyBase(double lat) 
    { 
        accessLatency = lat; 
    }
    
    /// @brief Get base (static) access latency
    ///
    /// @returns The current access latency
    ///
    
    double getAccessLatencyBase(void) 
    { 
        return accessLatency;
    }
    
    /// @brief Returns the total access latency for a source node 
    ///        accessing the channel
    ///
    /// @param src A subnet MAC data structure
    /// @param nodeId The source nodeId
    /// @param ifidx The source interface index
    ///
    /// @returns The total access latency for that node (in seconds) 
    ///          not including serialization delay
    ///
    
    double getAccessLatency(MacAneState* src, 
                            NodeAddress nodeId,
                            int ifidx) 
    { 
        // The current time translated to a double
        double now = (double)(src->myNode->getNodeTime()) / (double)SECOND; 
        
        // The offset (relative) time to next transmission opportunity
        double priorTransmissionOffset = firstAvailableTime - now;  
        
        // If the channel is now idle, then the access time is zero.
        if (priorTransmissionOffset < 0.0) 
        {
            priorTransmissionOffset = 0.0;
        }
        
        // Lock the channel until the transmission starts
        lockUntil(now + priorTransmissionOffset + accessLatency);
        
        return accessLatency + priorTransmissionOffset; 
    }
    
    /// @brief Lock channel until absolute time
    ///
    /// @param t Absolute time to lock channel 
    ///          (must be greater than firstAvailableTime)
    ///
    /// This routine is basically used by the MAC to determine 
    /// the next available transmit time.  It then has to 
    /// calculate the serialization time that it then 
    /// reports with lockFor().
    ///
    /// @see lockFor()
    ///
    
    void lockUntil(double t) 
    {
        if (t > firstAvailableTime) 
        {
            firstAvailableTime = t;
        }
    }
    
    /// @brief Lock channel for additional time from 
    ///        current firstAvailableTime
    ///
    /// @param dt Additional time (in seconds) to lock the channel. 
    ///           \f$dt \geq 0\f$
    ///
    /// This routine is used to lock the channel for additional 
    /// serialization delay that is not known until the 
    /// abstract MAC knows the other parameters from the subnet.  
    /// This routine is used in conjunction with lockUntil().
    ///
    /// @see lockUntil()
    ///
    
    void lockFor(double dt) 
    {
        ERROR_Assert(dt >= 0.0,
                     "The ANE model has detected that the"
                     " desired locking time on the current"
                     " upstream channel is less than zero."
                     " This indicates a logic error in"
                     " this QualNet model.  The simulation"
                     " cannot continue.");
        
        firstAvailableTime += dt;
    }
    
} ;

#endif                          /* __ANE_ANESAT_UPTREAM_H__ */
