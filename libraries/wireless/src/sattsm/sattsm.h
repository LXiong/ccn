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

#ifndef _SAT_TSM_H_
#define _SAT_TSM_H_

#include <list>

#include "api.h"
#include "util_constants.h"


///
/// @file sattsm.h
///
/// @brief The class header definition file for the SATTSM generic
/// classes
///

///
/// @def SATTSM_DEBUG
///
/// @brief Debugging macro for file
///

#ifdef SATTSM_DEBUG
# undef SATTSM_DEBUG
#endif /* SATTSM_DEBUG */

#define SATTSM_DEBUG 0


///
/// @namespace PhySattsm
///
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{
    // Forward declaration of Signal Record
    class SignalRecord;

    ///
    /// @class Gain
    ///
    /// @brief a generic gain element that has an output power
    ///
    /// More precisely an amplifier
    ///

    class Gain
    {
    public:

        ///
        /// @brief Empty constructor
        ///

        Gain()
        {

        }

        ///
        /// @brief Gain destructor
        ///

        ~Gain()
        {

        }

        ///
        /// @brief A routine to calculate the power of a signal
        /// received
        ///
        /// @param srec a reference to signal record
        ///
        /// @returns the output power in W
        ///

        virtual double getOutputPower(SignalRecord& srec) = 0;
    };

    ///
    /// @class SignalRecord
    ///
    /// @brief A class that implements a history of
    /// a primary signal (the signal)
    /// and all other signals recieved during the reception of that
    /// signal that constitute interference with the primary signal
    ///

    class SignalRecord
    {
    public:

        /// A pointer to the signal message (primary signal)
        Message* m_msg;

        /// The power of the signal in W
        double m_signalPower;

        ///
        /// The orientation of the signal in absolute terms (not
        /// relative to the direction of the antenna)
        ///
        Orientation m_direction;

        /// The time the signal started with reference to receiver
        clocktype m_timeStart;

        /// The time the signal ended with reference to receiver
        clocktype m_timeEnd;

        ///
        /// A list of signal records that records currently
        /// active signals
        ///

        std::list<SignalRecord> m_activeSignals;

        ///
        /// @brief Return the current input power of the signal in W
        ///
        /// @returns the input power in W
        ///

        double getInputPower()
        {
            double totalPower = getSignalPower() + getNoisePower();

            return totalPower;
        }

        ///
        /// @brief Calculate the input direction
        ///
        /// @returns The input direction with absolute reference
        ///

        Orientation getInputDirection()
        {
            return m_direction;
        }

        ///
        /// @brief returns the signal power of the signal
        ///
        /// @returns the signal power of the signal
        ///

        double getSignalPower()
        {
            return m_signalPower;
        }

        ///
        /// @brief returns the noise power of the signal
        ///
        /// @returns the noise power in W
        ///
        /// Currently the noise power is added by the antenna
        /// and this is called before the antenna adds the
        /// sky noise
        ///

        double getNoisePower()
        {
            return 0;
        }

        ///
        /// @class SimpleGain
        ///
        /// @brief A simple gain (amplifier) element
        ///
        /// Completely linear gain
        ///

        class SimpleGain : public Gain
        {

            /// Current gain of the amplifier in absolute terms
            double m_gain;

        public:

            ///
            /// @brief Constructor for gain element
            ///
            /// @brief gain Gain of amplifier
            ///

            SimpleGain(double gain = 1)
            : m_gain(gain)
            {

            }

            ///
            /// @brief Calculates the output power of a signal
            ///
            /// @param srec a reference to the current signal
            /// being received
            ///
            /// @returns The output power with respect to the
            /// current receiving object
            ///

            double getOutputPower(SignalRecord& srec)
            {
                return m_gain * srec.m_signalPower;
            }
        };

        ///
        /// @brief Calculate the overall interference power
        /// with respect to a gain element
        ///
        /// @param g a reference to a gain element
        ///
        /// @returns the total interference power in W
        ///

        double getInterferencePower(Gain& g)
        {
            std::list<SignalRecord>::iterator pos =
            m_activeSignals.begin();

            double interferencePower(0);
            for (; pos != m_activeSignals.end(); pos++)
            {
                SignalRecord& entry = *pos;

                if (isActiveSignal(entry))
                {
                    interferencePower += g.getOutputPower(entry);
                }
            }

            return interferencePower;
        }

        ///
        /// @brief Calculates the interference power with
        /// respect to a default amplifier (unity buffer amplifier)
        ///
        /// @returns Total interference power in W
        ///

        double getInterferencePower()
        {
            SimpleGain g;

            return getInterferencePower(g);
        }

        ///
        /// @brief Calculates the signal to noise ratio (SNR)
        /// with respect to a default amplifier (unity buffer amp)
        ///
        /// @returns SNR as a log ratio level
        ///
        /// @see getSnr(Gain& g)
        ///

        double getSnr()
        {
            SimpleGain g;

            return getSnr(g);
        }

        /// @brief Calculates the signal to noise ratio (SNR)
        /// with respect to a provided amplifier (usually a
        /// directional antenna)
        ///
        /// @param g a reference to a gain element
        ///
        /// @returns the SNR as an log ratio
        ///

        double getSnr(Gain& g)
        {
            double signalPower = getSignalPower();
            double noisePower = getNoisePower();
            double interferencePower = getInterferencePower(g);

            double nonSignalPower = noisePower + interferencePower;

            return UTIL::abs2db(signalPower / nonSignalPower);
        }

        ///
        /// @brief return whether or not the current signal record
        /// is equivalent to a given signal record
        ///
        /// @param rec a reference to a given signal record
        ///
        /// @returns True if the signal records refer to the same
        /// signal and false otherwise
        ///

        bool operator==(const SignalRecord& rec)
        {
            if (rec.m_msg == m_msg)
            {
                return true;
            }

            return false;
        }

        ///
        /// @brief return whether or not the current signal record
        /// is not equivalent to a given signal record
        ///
        /// @param rec a reference to a given signal record
        ///
        /// @returns False if the signal records refer to the same
        /// signal and true otherwise
        ///

        bool operator!=(const SignalRecord& srec)
        {
            return !operator==(srec);
        }

        ///
        /// @brief Constructor for a signal record given
        /// a reception information
        ///
        /// @param prop_rx a pointer to a signal reception data
        /// structure
        ///

        SignalRecord(PropRxInfo* prop_rx)
            : m_msg(prop_rx->txMsg),
            m_signalPower(prop_rx->rxPower_dBm),
            m_direction(prop_rx->rxDOA),
            m_timeStart(prop_rx->rxStartTime),
            m_timeEnd( CLOCKTYPE_MAX )
        {
        }

        ///
        /// @brief Destructor logic
        ///

        ~SignalRecord()
        {
            m_msg = NULL;
        }

        ///
        /// @brief Calculates whether a given signal is active
        /// at the specified time
        ///
        /// @param now the time reference for the signal calculation
        ///
        /// @returns True if the current signal is active, False
        /// otherwise
        ///
        /// @see isActiveSignal(SignalRecord& now)
        ///

        bool isActiveSignal(clocktype now)
        {
            if (m_timeStart > now)
            {
                return false;
            }
            else if (m_timeEnd < now)
            {
                return false;
            }

            return true;
        }

        ///
        /// @brief Calculates whether the current signal was
        /// active at the same time as the provided signal
        ///
        /// @param srec a reference to a signal record to
        /// compare activity against the current/local signal
        /// record
        ///
        /// @returns True is there is overlap between the two
        /// records and false otherwise
        ///
        /// Overlap is deemed to not have occurred if the provided
        /// ended before the reference signal started or
        /// started after the reference signal ended.  Otherwise
        /// it is deemed to have overlapped.
        ///

        bool isActiveSignal(SignalRecord& srec)
        {
            if (srec.m_timeEnd < m_timeStart)
            {
                return false;
            }
            else if (srec.m_timeStart > m_timeEnd)
            {
                return false;
            }

            return true;
        }

    } ;

    ///
    /// @class SignalList
    ///
    /// @brief A list of signals being received by an interface
    ///

    class SignalList
    {
        /// A list of signals in reception
        std::list<SignalRecord> m_list;

        ///
        /// @brief Update the signal list given new reception
        /// information
        ///
        /// @param propRxInfo a pointer to a received signal
        /// data structure
        /// @param now the reference time of the update
        /// @param updateEndTime a flag to update when the
        /// currently recieved signal (primary signal) is being
        /// received.
        /// @returns A signal record for the event
        ///
        /// The flag is used because it is possible for other
        /// signals to arrive during a given target reception.
        /// Therefore, it is impossible to know the end time of the
        /// signal until it has actually occurred especially since
        /// signals may be truncated by PHY calls in QualNet
        ///

        SignalRecord update(PropRxInfo* propRxInfo,
                            clocktype now,
                            bool updateEndTime = true)
        {
            std::list<SignalRecord>::iterator pos =
            m_list.begin();

            bool done(false);
            while (!done && pos != m_list.end())
            {
                if ((*pos).m_msg == propRxInfo->txMsg )
                {
                    done = true;
                }

                ++pos;
            }

            ERROR_Assert(done,
                         "The simulation cannot pop a signal that"
                         " was sent to the physical receiver.  This"
                         " is not possible unless the data structure"
                         " has been corrupted.  The simulation cannot"
                         " continue.");

            if (updateEndTime)
            {
                (*pos).m_timeEnd = now;
            }

            SignalRecord srec(*pos);

            pos = m_list.begin();

            while (pos != m_list.end())
            {
                if ((*pos).isActiveSignal(srec)
                     && (*pos) != srec)
                {
                    srec.m_activeSignals.push_back(*pos);
                }

            }

            gc(now);

            return srec;
        }

        ///
        /// @brief Perform garbage collection on the data structure
        ///
        /// @param now the reference time for the garbage collection
        ///
        /// This routine removes all signals that refer to a time
        /// strictly before the reference time.  That is, it determines
        /// what signals are still active as of the reference time
        /// and the earliest signal that matters to currently active
        /// signals.
        ///
        /// Then it removes all signals that refer to times before
        /// the earliest start time of an active signal
        ///
        /// This calculation is non-trivial and so should be
        /// optimized (hopefully removed) in a future release
        ///

        void gc(clocktype now)
        {
            std::list<SignalRecord>::iterator pos =
            m_list.begin();

            bool first(true);
            clocktype leastValidStartTime = 0;

            while (pos != m_list.end())
            {
                if ((*pos).isActiveSignal(now) )
                {
                    if (first)
                    {
                        leastValidStartTime = (*pos).m_timeStart;
                        first = false;
                    }
                    else if ((*pos).m_timeStart < leastValidStartTime)
                    {
                        leastValidStartTime = (*pos).m_timeStart;
                    }
                }

                ++pos;
            }

            pos = m_list.begin();
            while (pos != m_list.end())
            {
                if ((*pos).isActiveSignal(leastValidStartTime) == false)
                {
                    m_list.erase(pos);
                }

                ++pos;
            }
        }

    public:

        ///
        /// @brief Default empty constructor
        ///
        SignalList()
        {

        }

        ///
        /// @brief Process a new signal arrival event
        ///
        /// @param propRxInfo a pointer to the signal reception
        /// data
        ///

        void start(PropRxInfo* propRxInfo)
        {
            m_list.push_back(SignalRecord(propRxInfo));
        }

        ///
        /// @brief Sample the current reception environment
        /// with the given reception data at the given time
        ///
        /// @param propRxInfo a pointer to the current reception
        /// signal data
        /// @param now a reference time for the calculation
        ///
        /// @returns a signal record representing the current
        /// reception environment
        ///

        SignalRecord sample(PropRxInfo* propRxInfo,
                            clocktype now)
        {
            SignalRecord sample = update(propRxInfo,
                                         now,
                                         false);

            return sample;
        }

        ///
        /// @brief Process an signal end event
        ///
        /// @param propRxInfo a pointer to the current reception
        /// signal data
        ///
        /// @returns a signal record for the signal contained
        /// in the propRxInfo data structure
        ///
        /// The duration of the signal is now set and assumed to
        /// be properly reflected in the propRxInfo data structure
        ///

        SignalRecord end(PropRxInfo* propRxInfo)
        {
            clocktype now = propRxInfo->rxStartTime +
            propRxInfo->duration;

            SignalRecord srec = update(propRxInfo,
                                       now,
                                       true);

            return srec;
        }

    };


    ///
    /// @class ProcessingStage
    ///
    /// @brief A class that specifies a basic processing element
    /// within a node processing chain
    ///

    class ProcessingStage
    {

    public:

        ///
        /// @brief Default (empty) constructor
        ///

        ProcessingStage()
        {

        }

        ///
        /// Destructor
        ///

        ~ProcessingStage()
        {

        }

        ///
        /// @brief Handle a transmission (down stack) event in
        /// the processing stage
        ///
        /// @param node A pointer to the local node data structure
        /// @param ifidx The logical index of the local PHY process
        /// @param A pointer to a data structure holding the current
        /// frame undergoing transmission
        /// @param ok A return flag to indicate whether or not the
        /// current module processing stage is successful (true) or
        /// not (false)
        ///
        ///

        virtual void onSend(Node* node,
                            int phyIndex,
                            Message* msg,
                            bool& ok)
        {
            ok = false;

            ERROR_ReportError("This stage is not part of the"
                              " transmission chain.  Please verify"
                              " the program code that invokes this"
                              " method.  The simulation cannot"
                              " continue.");

        }

        ///
        /// @brief Process a receive (up stack) indication
        ///
        /// @param node a pointer to the local node data structure
        /// @param ifidx the logical interface index of the local
        /// PHY process
        /// @param msg a pointer to the frame undergoing reception
        /// @param ok a reference to a flag that indicates whether
        /// or not the current reception process stage concluded
        /// successfully or not
        /// @param srec a reference to the signal record for
        /// this frame with reference to the current time, node,
        /// and antenna configurations
        ///
        ///

        virtual void onReceive(Node* node,
                               int phyIndex,
                               Message* msg,
                               bool& ok,
                               const SignalRecord& srec)
        {
            ok = false;

            ERROR_ReportError("This stage is not part of the"
                              " receiver chain.  Please verify"
                              " the program code that invokes this"
                              " method.  The simulation cannot"
                              " continue.");

        }

    } ;

    ///
    /// @class TxProcessingChain
    ///
    /// @brief A tranmission (downstack) processing chain
    ///

    class TxProcessingChain
    {
        std::list<ProcessingStage*> m_chain;

    public:

        ///
        /// @brief Default (empty) constructor
        ///

        TxProcessingChain()
        {

        }

        ///
        /// @brief Class destructor
        ///

        ~TxProcessingChain()
        {

        }

        ///
        /// @brief Push a processing stage on the chain FIFO
        ///
        /// @param stage a pointer to a processing stage element
        ///
        /// Reference is pushing from top to bottom
        ///

        void push(ProcessingStage* stage)
        {
            m_chain.push_front(stage);
        }

        ///
        /// @brief Handle a transmission (down stack) event in
        /// the processing stage
        ///
        /// @param node A pointer to the local node data structure
        /// @param ifidx The logical index of the local PHY process
        /// @param A pointer to a data structure holding the current
        /// frame undergoing transmission
        /// @param ok A return flag to indicate whether or not the
        /// current module processing stage is successful (true) or
        /// not (false)
        ///
        /// @see ProcessingStage
        ///

        void onSend(Node* node,
                    int phyIndex,
                    Message* msg,
                    bool& ok)
        {
            std::list<ProcessingStage*>::iterator pos =
                m_chain.begin();

            ok = true;

            while (pos != m_chain.end())
            {
                ProcessingStage* stage = *pos;

                stage->onSend(node,
                              phyIndex,
                              msg,
                              ok);

                if (!ok)
                {
                    break;
                }
            }
        }
    };

    ///
    /// @class RxProcessingChain
    ///
    /// @brief A receiver chain specification
    ///

    class RxProcessingChain
    {
        /// A list of reciever processing stages
        std::list<ProcessingStage*> m_chain;

public:

        ///
        /// @brief Default (empty) constructor
        ///

        RxProcessingChain()
        {

        }

        ///
        /// Class destructor
        ///

        ~RxProcessingChain()
        {

        }

        ///
        /// @brief Push a processing stage on the chain FIFO
        ///
        /// @param stage a pointer to a processing stage element
        ///
        /// Reference is top to bottom
        ///

        void push(ProcessingStage* stage)
        {
            m_chain.push_front(stage);
        }

        ///
        /// @brief Process a receive (up stack) indication
        ///
        /// @param node a pointer to the local node data structure
        /// @param ifidx the logical interface index of the local
        /// PHY process
        /// @param msg a pointer to the frame undergoing reception
        /// @param ok a reference to a flag that indicates whether
        /// or not the current reception process stage concluded
        /// successfully or not
        /// @param srec a reference to the signal record for
        /// this frame with reference to the current time, node,
        /// and antenna configurations
        ///
        /// @see ProcessingStage
        ///
        /// Notice the use of the reverse iterator so that the
        /// transmission chain has the same reference direction
        ///
        /// @see TxProcessingChain
        ///

        void onReceive(Node* node,
                       int phyIndex,
                       Message* msg,
                       bool& ok,
                       const SignalRecord& srec)
        {
            std::list<ProcessingStage*>::reverse_iterator pos =
            m_chain.rbegin();

            ok = true;

            while (pos != m_chain.rend())
            {
                ProcessingStage* stage = *pos;

                stage->onReceive(node,
                                 phyIndex,
                                 msg,
                                 ok,
                                 srec);

                if (!ok)
                {
                    break;
                }
            }
        }

    };

    ///
    /// @class ProcessingChain
    ///
    /// @brief A bi-directional processing stage
    ///
    /// This class implements a pair of chains and the
    /// elements contained may refer to shared tx/rx elements
    /// since they are referenced by pointer values
    ///

    class ProcessingChain : public ProcessingStage
    {
        /// The receiver processing chain
        RxProcessingChain m_rx;

        /// The transmission processing chain
        TxProcessingChain m_tx;

    public:

        ///
        /// @brief Default (empty) constructor
        ///

        ProcessingChain()
        {

        }

        ///
        /// @brief Class destructor
        ///

        ~ProcessingChain()
        {

        }

        ///
        /// @brief Handle a transmission (down stack) event in
        /// the processing stage
        ///
        /// @param node A pointer to the local node data structure
        /// @param ifidx The logical index of the local PHY process
        /// @param A pointer to a data structure holding the current
        /// frame undergoing transmission
        /// @param ok A return flag to indicate whether or not the
        /// current module processing stage is successful (true) or
        /// not (false)
        ///
        /// @see ProcessingStage
        ///

        void onSend(Node* node,
                    int phyIndex,
                    Message* msg,
                    bool& ok)
        {
            m_tx.onSend(node,
                        phyIndex,
                        msg,
                        ok);
        }

        ///
        /// @brief Process a receive (up stack) indication
        ///
        /// @param node a pointer to the local node data structure
        /// @param ifidx the logical interface index of the local
        /// PHY process
        /// @param msg a pointer to the frame undergoing reception
        /// @param ok a reference to a flag that indicates whether
        /// or not the current reception process stage concluded
        /// successfully or not
        /// @param srec a reference to the signal record for
        /// this frame with reference to the current time, node,
        /// and antenna configurations
        ///
        /// @see ProcessingStage
        ///

        void onReceive(Node* node,
                       int phyIndex,
                       Message* msg,
                       bool& ok,
                       const SignalRecord& srec)
        {
            m_rx.onReceive(node,
                           phyIndex,
                           msg,
                           ok,
                           srec);
        }

        ///
        /// @brief Push a processing stage on the TX chain
        ///
        /// @param stage a pointer to a processing stage element
        ///

        void push_tx(ProcessingStage* stage)
        {
            m_tx.push(stage);
        }

        ///
        /// @brief Push a processing stage on the RX chain
        ///
        /// @param stage a pointer to a processing stage element
        ///

        void push_rx(ProcessingStage* stage)
        {
            m_rx.push(stage);
        }
    };

    ///
    /// @class ManagedEntity
    ///
    /// @brief The generic PHY expects a certain number of
    /// management routines to be implemented.
    ///
    /// While this is
    /// not all possible permuations, it is a sample set based on
    /// the necessary capabilities of most MAC protocols
    ///

    class ManagedEntity
    {
    public:

        ///
        /// @brief Default (empty) constructor
        ///

        ManagedEntity()
        {

        }

        ///
        /// @brief Class destructor
        ///

        ~ManagedEntity()
        {

        }

        ///
        /// @brief Calculate tranmission duration of abstract
        /// frame size provided
        ///
        /// @param sizeInBits Size of abstract frame in bits
        ///
        /// @returns the total length of transmission in seconds
        ///

        virtual clocktype getTransmissionDuration(int sizeInBits) = 0;

        ///
        /// @brief Requests the current tranmission chain to
        /// change its power level
        ///
        /// @param power_mW specified power in mW
        ///

        virtual void setTransmitPower(double power_mW) = 0;

        ///
        /// @brief Requests the processing chain to provide the
        /// current output power
        ///
        /// @returns the current transmit power in mW
        ///

        virtual double getTransmitPower() = 0;

        ///
        /// @brief Requests the processing chain to return the
        /// current transmission rate
        ///
        /// @returns the current transmission rate in bits/second
        ///
        /// This represents the raw transmission rate of the modulated
        /// channel and does not account for user headers, coding,
        /// guard times, synchronization, etc.
        ///

        virtual double getTransmitRate() = 0;

        ///
        /// @brief Requests the processing chain to return the
        /// current receive rate
        ///
        ///
        /// @returns the current transmission rate in bits/second
        ///
        /// This represents the raw reception rate of the modulated
        /// channel and does not account for user headers, coding,
        /// guard times, synchronization, etc.
        ///

        virtual double getReceiveRate() = 0;
    };

    ///
    /// @class PhyEntity
    ///
    /// @brief Join class for Physical layer entity
    ///

    class PhyEntity : public ProcessingChain, public ManagedEntity
    {

    };

}


#endif                          /* _SAT_TSM_H_ */
