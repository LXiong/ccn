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
// Purpose: Simulate the MPLS Label Stack Encoding RFC 3032 "MPLS SHIM"
//

#ifndef MPLS_SHIM_H
#define MPLS_SHIM_H

//
// Reserved Label Values
//
typedef enum {
    IPv4_Explicit_NULL_Label = 0, // only legal at bottom of label stack
                                  // indicates that label stack needs to
                                  // be popped, and routing performed based
                                  // on IPv4 header

    Router_Alert_Label = 1,       // legal anywhere except bottom of stack
                                  // delivered to a local software module
                                  // for processing before being routed
                                  // according to the label below this one

    IPv6_Explicit_NULL_Label = 2, // only legal at bottom of label stack
                                  // indicates that label stack needs to
                                  // be popped, and routing performed based
                                  // on IPv6 header

    Implicit_NULL_Label = 3,      // never actually appears in the
                                  // encapsulation, but specified in the
                                  // LDP and causes the LSR to pop the
                                  // stack instead of doing the replacement

    Last_Reserved = 15            // 4 - 15 also reserved
} Mpls_Shim_Reserved_Label;

//
// Actual Label Header
//
typedef struct {
    UInt32 shimStack;
                 //Label:20,      // Label Value
                 //Exp:3,         // Reserved for Experimental Use
                 //S:1,           // If Bottom of Stack, S == 1, 0 otherwise
                 //TTL:8;         // Time to Live
} Mpls_Shim_LabelStackEntry;

//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntrySetLabel()
//
// PURPOSE      : Set the value of Label for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                            Exp,S and TTL
//                Label     - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void Mpls_Shim_LabelStackEntrySetLabel(UInt32 *shimStack,
                                              UInt32 Label)
{
    //masks Label within boundry range
    Label = Label & maskInt(13, 32);

    //clears first 20 bits
    *shimStack = *shimStack & (maskInt(21, 32));

    //setting the value of label to shim stack
    *shimStack = *shimStack | LshiftInt(Label, 20);
}



//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntrySetExp()
//
// PURPOSE      : Set the value of Exp for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                            Exp,S and TTL
//                Exp       - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void Mpls_Shim_LabelStackEntrySetExp(UInt32 *shimStack, UInt32 Exp)
{
    //masks Exp within boundry range
    Exp = Exp & maskInt(30, 32);

    //clears bits 21-23
    *shimStack = *shimStack & (~(maskInt(21, 23)));

    //setting the value of Exp to shim stack
    *shimStack = *shimStack | LshiftInt(Exp, 23);
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntrySetS()
//
// PURPOSE      : Set the value of S for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                            Exp,S and TTL
//                S         - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void Mpls_Shim_LabelStackEntrySetS(UInt32 *shimStack, BOOL S)
{
    unsigned int x = S;

    //masks S within boundry range
    x = x & maskInt(32, 32);

    //clears the 24th bit
    *shimStack = *shimStack & (~(maskInt(24, 24)));

    //setting the value of x in shim stack
    *shimStack = *shimStack | LshiftInt(x, 24);
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntrySetTTL()
//
// PURPOSE      : Set the value of TTL for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                            Exp,S and TTL
//                TTL       - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void Mpls_Shim_LabelStackEntrySetTTL(UInt32 *shimStack, UInt32 TTL)
{
    //masks TTL within boundry range
    TTL = TTL & maskInt(25, 32);

    //clears the last 8 bits
    *shimStack = *shimStack & maskInt(1, 24);

    //setting the value of TTL in shim stack
    *shimStack = *shimStack | TTL;
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntryGetTTL()
//
// PURPOSE      : Returns the value of TTL for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                             Exp,S and TTL
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 Mpls_Shim_LabelStackEntryGetTTL(UInt32 shimStack)
{
    UInt32 TTL = shimStack;

    //clears the first 24 bits
    TTL = shimStack & maskInt(25, 32);

    return TTL;
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntryGetLabel()
//
// PURPOSE      : Returns the value of Label for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                             Exp,S and TTL
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 Mpls_Shim_LabelStackEntryGetLabel(UInt32 shimStack)
{
    UInt32 Label = shimStack;

    //clears the last 12 bits
    Label = Label & maskInt(1, 20);

    //right shifts 12 places so that last 20 bits represent label
    Label = RshiftInt(Label, 20);

    return Label;
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntryGetExp()
//
// PURPOSE      : Returns the value of Exp for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                             Exp,S and TTL
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 Mpls_Shim_LabelStackEntryGetExp(UInt32 shimStack)
{
    UInt32 Exp = shimStack;

    //clears all the bits except 21-23
    Exp = Exp & maskInt(21, 23);

    //right shifts 9 places so that last 3 bits represent Exp
    Exp = RshiftInt(Exp, 23);

    return Exp;
}


//-------------------------------------------------------------------------
// FUNCTION     : Mpls_Shim_LabelStackEntryGetS()
//
// PURPOSE      : Returns the value of S for Mpls_Shim_LabelStackEntry
//
// PARAMETERS   : shimStack - The variable containing the value of Label,
//                             Exp,S and TTL
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL Mpls_Shim_LabelStackEntryGetS(unsigned int shimStack)
{
    UInt32 S = shimStack;

    //clears all the bits except 24th
    S = S & maskInt(24, 24);

    //right shifts 8 places so that last bit contains the value of S
    S = RshiftInt(S, 24);

    return (BOOL)S;
}


static void
MplsShimModifyLabel(
    Mpls_Shim_LabelStackEntry *shim_label,
    unsigned int Label,
    BOOL bottomOfStack,
    unsigned int TTL)
{
    Mpls_Shim_LabelStackEntrySetLabel(&(shim_label->shimStack), Label);
    if (bottomOfStack)
    {
        Mpls_Shim_LabelStackEntrySetS(&(shim_label->shimStack), 1) ;
    }
    else
    {
        Mpls_Shim_LabelStackEntrySetS(&(shim_label->shimStack), 0);
    }

    Mpls_Shim_LabelStackEntrySetTTL(&(shim_label->shimStack), TTL);
}


//-------------------------------------------------------------------------
// FUNCTION     : MplsShimFreeLabel()
//
// PURPOSE      : Freeing an temporarily allocated label. The label
//                should allocated by MplsShimCreateLabel() function
//
// PARAMETERS   : shim_label - pointer to pointer to the label to be freed
//
// RETURN VALUE : void
//
// ASSUMPTION   : none.
//-------------------------------------------------------------------------
static void
MplsShimFreeLabel(Mpls_Shim_LabelStackEntry **shim_label)
{
    if (*shim_label != NULL)
    {
        // check if label exists
        MEM_free(*shim_label);
    }
}


static void
MplsShimCreateLabel(
    Mpls_Shim_LabelStackEntry *shim_label,
    unsigned int Label,
    BOOL bottomOfStack,
    unsigned int TTL)
{
    MplsShimModifyLabel(
        shim_label,
        Label,
        bottomOfStack,
        TTL);
}


static void
MplsShimAddLabel(
    Node *node,
    Message *msg,
    unsigned int Label,
    BOOL bottomOfStack,
    unsigned int TTL)
{
    Mpls_Shim_LabelStackEntry shim_label = {(unsigned int) 0};
    MplsShimCreateLabel(&shim_label, Label, bottomOfStack, TTL);

    MESSAGE_AddHeader(node,
                      msg,
                      sizeof(Mpls_Shim_LabelStackEntry),
                      TRACE_MPLS_SHIM);

    memcpy(MESSAGE_ReturnPacket(msg), &shim_label,
           sizeof(Mpls_Shim_LabelStackEntry));
}


static void
MplsShimReplaceLabel(
    Node *node,
    Message *msg,
    unsigned int Label,
    BOOL bottomOfStack,
    unsigned int TTL)
{
    Mpls_Shim_LabelStackEntry shim_label = {(unsigned int) 0};
    MplsShimCreateLabel(&shim_label, Label, bottomOfStack, TTL);

    memcpy(MESSAGE_ReturnPacket(msg), &shim_label,
           sizeof(Mpls_Shim_LabelStackEntry));
}


static BOOL
MplsShimBottomOfStack(
    Mpls_Shim_LabelStackEntry *shim_label)
{
    return Mpls_Shim_LabelStackEntryGetS(shim_label->shimStack) == 1;
}


static void
MplsShimPopTopLabel(
    Node *node,
    Message *msg,
    BOOL *bottomOfStack,
    Mpls_Shim_LabelStackEntry *shim_label)
{
    memcpy(shim_label, MESSAGE_ReturnPacket(msg),
           sizeof(Mpls_Shim_LabelStackEntry));

    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(Mpls_Shim_LabelStackEntry),
                         TRACE_MPLS_SHIM);

    *bottomOfStack = MplsShimBottomOfStack(shim_label);
}

static void
MplsShimReturnTopLabel(
    Message *msg,
    Mpls_Shim_LabelStackEntry *shim_label)
{
    memcpy(shim_label, MESSAGE_ReturnPacket(msg),
           sizeof(Mpls_Shim_LabelStackEntry));
}

#endif // MPLS_SHIM_H

