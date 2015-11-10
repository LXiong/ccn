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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"
#include "routing_star.h"

#include "network_neighbor_prot.h"


//----- Notes -----//

// This implementation is based on
//     J.J. Garcia-Luna-Aceves and M. Spohn,
//     "Source-Tree Routing in Wireless Networks,",
//     Proc. IEEE ICNP 99: 7th International Conference on Network Protocols,
//     Toronto, Canada, October 31--November 3, 1999.
//
// http://www.cse.ucsc.edu/research/ccrg/publications/spohn.icnp99.pdf
//
// Comments in quotes are from the above source document.
// Comments without quotes are mine.
//
// The one major assumption made is the following:
//     If a node receives an LSU from a neighbor that the Neighbor Protocol
//     has not yet notified STAR about, the node calls the NeighborUp procedure
//     first, and then processes the received LSUs.  This behavior is the
//     equivalent of having the Neighbor Protocol notify STAR just before the
//     reception of the LSUs.  The alternative to this is to add the
//     LSU describing this previously unknown neighbor to the list of LSUs
//     actually received from this neighbor, and that might be more efficient,
//     although it is too large a deviation from the actual source documents
//     to be included here.
//
// Another two minor assumptions are the additional check to make sure that the
//     BuildShortestPathTree function does not ever re-insert the head node,
//     and the assignment of the newly calculated shortest path tree to STi'
//     in ORA mode (these were probably inadvertently left off the pseudocode).
//


#define INFINITY_COST 10000
#define STAR_CURRENT_CLOCK() ((int) (node->getNodeTime() / MILLI_SECOND))
#define NUM_NEIGHBORS(star) (star->topologyGraph_i.edgeRoot->numNeighbors)
#define RETURN_NEIGHBOR_ADDRESS(star, index) \
            (star->topologyGraph_i.edgeRoot->neighbors[index]->ipAddr_v)


#define INITIAL_NUM_ENTRIES 10
#define DEFAULT_LINK_COST   1
#define DEFAULT_DELTA       (INFINITY_COST / 2)
#define MAX_LSUS_IN_PACKET  20
#define STAR_JITTER         (100 * MILLI_SECOND)

//----- Static Function Declarations -----//

//
// FUNCTION     StarInitStats()
// PURPOSE      Initialize STAR Routing Protocol Dataspace
// PARAMETERS   myRoute            - pointer for dataspace
//              nodeInput
//

static
void StarInitStats(StarStats *stats);


//
// FUNCTION     NodeUp()        - Figure 1. STAR Specification
// PURPOSE      Initialize STAR state variables
// PARAMETERS   star            - pointer for dataspace
//

static void
NodeUp(
    StarData *star,
    NodeAddress ipAddr,
    Node *node);


static void
InitializeTopologyGraph(
    StarTopologyGraph *topologyGraph,
    NodeAddress ipAddr_root,
    Node *node);


static StarTopologyGraphEdgeEntry *
AddEdgeToTopologyGraph(
    StarTopologyGraph *topoGraph,
    NodeAddress ipAddr_u,
    NodeAddress ipAddr_v,
    int link_cost_l,
    int timestamp_t,
    Node *node);

static void
PrintTopologyGraph(
    NodeAddress rootAddr,
    StarTopologyGraph *topoGraph);


static StarTopologyGraphEdgeEntry *
CreateNewTopologyGraphEdgeEntry(
    NodeAddress ipAddr_u,
    NodeAddress ipAddr_v,
    int link_cost_l,
    int timestamp_t,
    Node *node)
{
    StarTopologyGraphEdgeEntry *Entry =
        (StarTopologyGraphEdgeEntry *)
        MEM_malloc(sizeof(StarTopologyGraphEdgeEntry));

    Entry->ipAddr_u = ipAddr_u;
    Entry->ipAddr_v = ipAddr_v;
    Entry->link_cost_l = link_cost_l;
    Entry->timestamp_t = timestamp_t;
    Entry->distance_d = INFINITY_COST;
    Entry->del = FALSE;

    Entry->neighbors =
        (StarTopologyGraphEdgeEntry **)
        MEM_malloc(
            sizeof(StarTopologyGraphEdgeEntry *) * INITIAL_NUM_ENTRIES);
    Entry->numNeighbors = 0;
    Entry->maxNeighbors = INITIAL_NUM_ENTRIES;

    return Entry;
}


//
// FUNCTION     StarInitStats()
// PURPOSE      Initialize STAR Routing Protocol Dataspace
// PARAMETERS   myRoute            - pointer for dataspace
//              nodeInput
//

static
void StarInitStats(StarStats *stats)
{
    stats->numLSU_sent = 0;
    stats->numLSU_recvd = 0;
    stats->numPkts_sent = 0;
    stats->numPkts_recvd = 0;
}


static void
EmptyTopologyGraph(
    StarTopologyGraph *topoGraph,
    Node *node)
{
    int i;

    topoGraph->edgeRoot = NULL;

    if (topoGraph->maxEdges > 0)
    {
        for (i = 0; i < topoGraph->numEdges; i++)
        {
           MEM_free(topoGraph->listOfEdges[i]->neighbors);
           MEM_free(topoGraph->listOfEdges[i]);
        }
        MEM_free(topoGraph->listOfEdges);

        topoGraph->listOfEdges = NULL;
        topoGraph->numEdges = 0;
        topoGraph->maxEdges = 0;
    }

    if (topoGraph->maxVertexEntries > 0)
    {
        for (i = 0; i < topoGraph->numVertexEntries; i++)
        {
           MEM_free(topoGraph->vertexEntries[i]);
        }
        MEM_free(topoGraph->vertexEntries);
        topoGraph->vertexEntries = NULL;
        topoGraph->numVertexEntries = 0;
        topoGraph->maxVertexEntries = 0;
    }
}


static void
ReplaceShortestPathTree(
    StarTopologyGraph *shortestPathTree,
    StarTopologyGraph *newST,
    Node *node)
{
    int i;

    EmptyTopologyGraph(shortestPathTree, node);

    InitializeTopologyGraph(shortestPathTree, newST->edgeRoot->ipAddr_v, node);

    for (i = 1; i < newST->numEdges; i++)
    {
        AddEdgeToTopologyGraph(
            shortestPathTree,
            newST->listOfEdges[i]->ipAddr_u,
            newST->listOfEdges[i]->ipAddr_v,
            newST->listOfEdges[i]->link_cost_l,
            newST->listOfEdges[i]->timestamp_t,
            node);
    }
}


static void
AddEdgeToListOfEdges(
    StarTopologyGraph *topoGraph,
    StarTopologyGraphEdgeEntry *edgeToAdd,
    Node *node)
{
    int numEntries = topoGraph->numEdges;

    if (numEntries == topoGraph->maxEdges)
    {
        StarTopologyGraphEdgeEntry **newListOfEdges;

        newListOfEdges = (StarTopologyGraphEdgeEntry **)
                         MEM_malloc(
                             (sizeof(StarTopologyGraphEdgeEntry *)
                              * (numEntries + INITIAL_NUM_ENTRIES)));

        if (numEntries > 0)
        {
            memcpy(newListOfEdges, topoGraph->listOfEdges,
                   (sizeof(StarTopologyGraphEdgeEntry *) * numEntries));

            MEM_free(topoGraph->listOfEdges);
        }
        topoGraph->listOfEdges = newListOfEdges;

        topoGraph->maxEdges += INITIAL_NUM_ENTRIES;
    }

    topoGraph->listOfEdges[numEntries] = edgeToAdd;

    topoGraph->numEdges++;
}


static void
InitializeTopologyGraph(
    StarTopologyGraph *topologyGraph,
    NodeAddress ipAddr_root,
    Node *node)
{
   topologyGraph->edgeRoot = CreateNewTopologyGraphEdgeEntry(
                                  ipAddr_root, // ipAddr_u
                                  ipAddr_root, // ipAddr_v
                                  0,           // link cost
                                  0,           // seq num
                                  node);

    topologyGraph->vertexEntries =
        (StarTopologyGraphVertexEntry **)
        MEM_malloc(sizeof(StarTopologyGraphVertexEntry *) *
                          INITIAL_NUM_ENTRIES);
    topologyGraph->numVertexEntries = 0;
    topologyGraph->maxVertexEntries = INITIAL_NUM_ENTRIES;

    topologyGraph->listOfEdges =
        (StarTopologyGraphEdgeEntry **)
        MEM_malloc(sizeof(StarTopologyGraphEdgeEntry *) *
                          INITIAL_NUM_ENTRIES);

    topologyGraph->numEdges = 0;
    topologyGraph->maxEdges = INITIAL_NUM_ENTRIES;

    AddEdgeToListOfEdges(topologyGraph, topologyGraph->edgeRoot, node);
}


//
// FUNCTION     NodeUp()        - Figure 1. STAR Specification
// PURPOSE      Initialize STAR state variables
// PARAMETERS   star            - pointer for dataspace
//

static void
NodeUp(
    StarData *star,
    NodeAddress ipAddr,
    Node *node)
{
    // "STi  <-- 0;"
    InitializeTopologyGraph(&(star->topoGraphST), ipAddr, node);

    // "STi' <-- 0;"
    InitializeTopologyGraph(&(star->topoGraphST_prime), ipAddr, node);

    // "Mi   <-- FALSE;"
    star->Mi = FALSE;

    // "NSi <-- FALSE;"
    star->NSi = FALSE;

    // "Ni   <-- 0;"
    // "TGi  <-- 0;"
    InitializeTopologyGraph(&(star->topologyGraph_i), ipAddr, node);

    star->seq_num = 0;
    star->delta = DEFAULT_DELTA;
    star->ipAddr_i = ipAddr;

    star->MSGi = (StarLSU *)
        MEM_malloc(sizeof(StarLSU) * INITIAL_NUM_ENTRIES);
    star->MSGi_numEntries = 0;
    star->MSGi_maxEntries = INITIAL_NUM_ENTRIES;

    star->topoGraph_k = (StarTopologyGraph *)
                        MEM_malloc(sizeof(StarTopologyGraph)
                                   * INITIAL_NUM_ENTRIES);
    star->numTopoGraph_k = 0;
    star->maxTopoGraph_k = INITIAL_NUM_ENTRIES;

    star->ShortestPathTree_k = (StarTopologyGraph *)
                               MEM_malloc(sizeof(StarTopologyGraph)
                                          * INITIAL_NUM_ENTRIES);
    star->numST_k = 0;
    star->maxST_k = INITIAL_NUM_ENTRIES;

}


static StarTopologyGraphVertexEntry *
AddVertexToTopologyGraph(
    StarTopologyGraph *topologyGraph,
    NodeAddress ipAddr_vertex,
    NodeAddress ipAddr_pred,
    NodeAddress ipAddr_suc,
    int       distance_d,
    NodeAddress flag_nbr,
    Node *node)
{
    StarTopologyGraphVertexEntry *vertexEntry;
    int numEntries = topologyGraph->numVertexEntries;

#ifdef DEBUG
    printf("\tAddVertexToTopologyGraph(root = %d vertex = %d / pred = %d)\n",
           topologyGraph->edgeRoot->ipAddr_v, ipAddr_vertex, ipAddr_pred);
#endif

    if (numEntries == topologyGraph->maxVertexEntries)
    {
        StarTopologyGraphVertexEntry **newEntries;

        newEntries = (StarTopologyGraphVertexEntry **)
                     MEM_malloc(
                         sizeof(StarTopologyGraphVertexEntry *)
                         * (numEntries + INITIAL_NUM_ENTRIES));

        if (numEntries > 0)
        {
            memcpy(newEntries, topologyGraph->vertexEntries,
                   sizeof(StarTopologyGraphVertexEntry *) * numEntries);

            MEM_free(topologyGraph->vertexEntries);
        }

        topologyGraph->vertexEntries = newEntries;

        topologyGraph->maxVertexEntries += INITIAL_NUM_ENTRIES;
    }

    vertexEntry = (StarTopologyGraphVertexEntry *)
                 MEM_malloc(sizeof(StarTopologyGraphVertexEntry));

    vertexEntry->ipAddr_vertex = ipAddr_vertex;
    vertexEntry->ipAddr_pred = ipAddr_pred;
    vertexEntry->ipAddr_suc = ipAddr_suc;
    vertexEntry->ipAddr_suc_prime = (NodeAddress) NULL;
    vertexEntry->distance_d = distance_d;
    vertexEntry->distance_d_prime = INFINITY_COST;
    vertexEntry->distance_d_double_prime = INFINITY_COST;
    vertexEntry->flag_nbr = flag_nbr;
    vertexEntry->valid = TRUE;
    vertexEntry->extracted = FALSE;

    topologyGraph->vertexEntries[numEntries] = vertexEntry;

    topologyGraph->numVertexEntries++;

    return vertexEntry;
}


static void
AddNeighborEntryToTopologyGraphEdgeEntry(
    StarTopologyGraphEdgeEntry *parent,
    StarTopologyGraphEdgeEntry *child,
    Node *node)
{
    int numEntries = parent->numNeighbors;

    if (numEntries == parent->maxNeighbors)
    {
        StarTopologyGraphEdgeEntry **newNeighborList;

        newNeighborList = (StarTopologyGraphEdgeEntry **)
                          MEM_malloc(
                              sizeof(StarTopologyGraphEdgeEntry *) *
                              (numEntries + INITIAL_NUM_ENTRIES));

        memcpy(newNeighborList, parent->neighbors,
               sizeof(StarTopologyGraphEdgeEntry *) * numEntries);

        MEM_free(parent->neighbors);

        parent->neighbors = newNeighborList;
        parent->maxNeighbors += INITIAL_NUM_ENTRIES;
    }

    parent->neighbors[numEntries] = child;
    parent->numNeighbors++;
}


static StarTopologyGraph *
GenericReturnTopologyGraph(
    StarTopologyGraph **topoGraph,
    int numEntries,
    NodeAddress ipAddr_k)
{
    int i;

    for (i = 0; i < numEntries; i++)
    {
        if ((*topoGraph)[i].edgeRoot->ipAddr_v == ipAddr_k)
        {
            return &((*topoGraph)[i]);
        }
    }

    return NULL;
}


static StarTopologyGraph *
ReturnTopologyGraph(
    StarData *star,
    NodeAddress k)
{
    if (star->ipAddr_i == k)
    {
        return &(star->topologyGraph_i);
    }

    return GenericReturnTopologyGraph(
               &(star->topoGraph_k),
               star->numTopoGraph_k,
               k);
}


static StarTopologyGraph *
ReturnShortestPathTree(
    StarData *star,
    NodeAddress k)
{
    if (star->ipAddr_i == k)
    {
        return &(star->topoGraphST);
    }

    return GenericReturnTopologyGraph(
               &(star->ShortestPathTree_k),
               star->numST_k,
               k);
}


static StarTopologyGraphEdgeEntry *
ReturnNeighborEdgeEntry(
    StarTopologyGraphEdgeEntry *parent,
    NodeAddress ipAddr)
{
    int i;

    for (i = 0; i < parent->numNeighbors; i++)
    {
        if (parent->neighbors[i]->ipAddr_v == ipAddr)
        {
            return parent->neighbors[i];
        }
    }

    return NULL;
}


static StarTopologyGraphEdgeEntry *
ReturnTopologyGraphEdgeParentEntry(
    StarTopologyGraph *topologyGraph,
    NodeAddress ipAddr_v)
{
    int i;

    for (i = 0; i < topologyGraph->numEdges; i++)
    {
        if (topologyGraph->listOfEdges[i]->ipAddr_v == ipAddr_v)
        {
            return topologyGraph->listOfEdges[i];
        }
    }

    return NULL;
}


static StarTopologyGraphEdgeEntry *
ReturnTopologyGraphEdgeChildEntry(
    StarTopologyGraph *topologyGraph,
    NodeAddress ipAddr_u,
    NodeAddress ipAddr_v)
{
    int i;

    for (i = 0; i < topologyGraph->numEdges; i++)
    {
        if (topologyGraph->listOfEdges[i]->ipAddr_u == ipAddr_u &&
            topologyGraph->listOfEdges[i]->ipAddr_v == ipAddr_v)
        {
            return topologyGraph->listOfEdges[i];
        }
    }

    return NULL;
}


static StarTopologyGraphVertexEntry *
ReturnTopologyGraphVertexEntry(
    StarTopologyGraph *topologyGraph,
    NodeAddress k)
{
    int i;

    for (i = 0; i < topologyGraph->numVertexEntries; i++)
    {
        if ((topologyGraph->vertexEntries[i]->ipAddr_vertex == k) &&
            (topologyGraph->vertexEntries[i]->valid == TRUE))
        {
            return topologyGraph->vertexEntries[i];
        }
    }

    return NULL;
}


static void
ConnectEdgeInTopologyGraph(
    StarTopologyGraph *topoGraph,
    StarTopologyGraphEdgeEntry *edgeToAdd,
    Node *node)
{
    int i;

    for (i = 0; i < topoGraph->numEdges; i++)
    {

         if (topoGraph->listOfEdges[i] != NULL)
            {
                if (topoGraph->listOfEdges[i]->ipAddr_v == edgeToAdd->ipAddr_u)
                {
                    if ( !ReturnNeighborEdgeEntry(
                     topoGraph->listOfEdges[i],
                     edgeToAdd->ipAddr_v))
                    {
                        AddNeighborEntryToTopologyGraphEdgeEntry(
                        topoGraph->listOfEdges[i],
                        edgeToAdd,
                        node);
                    }
                }
            }
    }
}


static StarTopologyGraphEdgeEntry *
AddEdgeToTopologyGraph(
    StarTopologyGraph *topoGraph,
    NodeAddress ipAddr_u,
    NodeAddress ipAddr_v,
    int link_cost_l,
    int timestamp_t,
    Node *node)
{
    StarTopologyGraphEdgeEntry *edgeToAdd;

#ifdef DEBUG
    printf("\tAddEdgeToTopologyGraph(k=%d, u=%d, v=%d, l=%d, t=%d)\n",
           topoGraph->edgeRoot->ipAddr_v,
           ipAddr_u, ipAddr_v, link_cost_l, timestamp_t);
#endif

    edgeToAdd = ReturnTopologyGraphEdgeChildEntry(
                      topoGraph,
                      ipAddr_u,
                      ipAddr_v);

    if (!edgeToAdd)
    {
        StarTopologyGraphEdgeEntry *existingEdge;

        existingEdge = ReturnTopologyGraphEdgeParentEntry(
                           topoGraph,
                           ipAddr_v);

        edgeToAdd = CreateNewTopologyGraphEdgeEntry(
                       ipAddr_u,         // ipAddr_u
                       ipAddr_v,         // ipAddr_v
                       link_cost_l,      // link_cost_l
                       timestamp_t,      // timestamp_t
                       node);


        AddEdgeToListOfEdges(topoGraph, edgeToAdd, node);

        if (existingEdge)
        {
            int i;

            for (i = 0; i < existingEdge->numNeighbors; i++)
            {
                AddNeighborEntryToTopologyGraphEdgeEntry(
                    edgeToAdd,
                    existingEdge->neighbors[i],
                    node);
            }

        }
    }

    ConnectEdgeInTopologyGraph(topoGraph, edgeToAdd, node);

    return edgeToAdd;
}


static void
AddNeighborToNeighborSet(
    StarData *star,
    NodeAddress neighbor,
    int link_cost,
    int timestamp,
    Node *node)
{
    AddEdgeToTopologyGraph(
        &(star->topologyGraph_i),
        star->ipAddr_i,
        neighbor,
        link_cost,
        timestamp,
        node);
}


void
AddLSUToMSGi(
    StarData *star,
    NodeAddress ipAddr_u,    // "An LSU for a link (u,v)"
    NodeAddress ipAddr_v,
    int       link_cost_l,   // "where l represents the cost of the link"
    int       timestamp_t)   // "and t is the timestamp assigned to the LSU"
{
    int numEntries = star->MSGi_numEntries;
    int i;

    // I need to add a check for whether or not an entry has already
    // been added due to the recursive nature of the topology graph traversal
    for (i = 0; i < numEntries; i++)
    {
        if (star->MSGi[i].ipAddr_u == ipAddr_u &&
            star->MSGi[i].ipAddr_v == ipAddr_v &&
            star->MSGi[i].link_cost_l == link_cost_l &&
            star->MSGi[i].timestamp_t == timestamp_t)
        {
            return;
        }
    }

    if (numEntries == star->MSGi_maxEntries)
    {
        StarLSU* nextMSGi;

        nextMSGi = (StarLSU *)
                   MEM_malloc(sizeof(StarLSU)
                              * (numEntries + INITIAL_NUM_ENTRIES));

        memcpy(nextMSGi, star->MSGi, sizeof(StarLSU) * numEntries);

        MEM_free(star->MSGi);
        star->MSGi = nextMSGi;
        star->MSGi_maxEntries += INITIAL_NUM_ENTRIES;
    }

#ifdef DEBUG
    printf("\tMSGi <-- MSGi U { (%d, %d, %d, %d) }\n",
           ipAddr_u,
           ipAddr_v,
           link_cost_l,
           timestamp_t);
#endif

    star->MSGi[numEntries].ipAddr_u = ipAddr_u;
    star->MSGi[numEntries].ipAddr_v = ipAddr_v;
    star->MSGi[numEntries].link_cost_l = link_cost_l;
    star->MSGi[numEntries].timestamp_t = timestamp_t;

    star->MSGi_numEntries++;
}


static void
PrintTopologyGraph(
    NodeAddress rootAddr,
    StarTopologyGraph *topoGraph)
{
    int i,j;

    for (i = 0; i < topoGraph->numEdges; i++)
    {
        printf("%d -> %d [%p]\n",
               topoGraph->listOfEdges[i]->ipAddr_u,
               topoGraph->listOfEdges[i]->ipAddr_v,
               topoGraph->listOfEdges[i]);
        for (j = 0; j < topoGraph->listOfEdges[i]->numNeighbors; j++)
        {
            printf("  %d -> %d [%p]\n",
                   topoGraph->listOfEdges[i]->neighbors[j]->ipAddr_u,
                   topoGraph->listOfEdges[i]->neighbors[j]->ipAddr_v,
                   topoGraph->listOfEdges[i]->neighbors[j]);
        }
    }
}


static void
GarbageCollector(
    StarTopologyGraph *topoGraph,
    StarTopologyGraphEdgeEntry *orphan)
{
    StarTopologyGraphEdgeEntry *graftPoint;
    graftPoint = ReturnTopologyGraphEdgeParentEntry(
                      topoGraph,
                      orphan->ipAddr_v);

    if (!graftPoint)
    {
        int i;

        for (i = 0; i < orphan->numNeighbors; i++)
        {
            GarbageCollector(topoGraph, orphan->neighbors[i]);
        }
        MEM_free(orphan);
    }
}


static void
DeleteEdge(
    StarTopologyGraph *topoGraph,
    StarTopologyGraphEdgeEntry *parent,
    int index)
{

    StarTopologyGraphEdgeEntry *orphan = parent->neighbors[index];
    int i;

    for (i = index + 1; i < parent->numNeighbors; i++)
    {
        parent->neighbors[i-1] = parent->neighbors[i];
    }
    parent->numNeighbors--;

    orphan = NULL;
}


static void
PruneEntryFromTopologyGraph(
    StarTopologyGraph *topoGraph,
    StarTopologyGraphEdgeEntry *edgeToDelete)
{
    int i;

    for (i = 0; i < topoGraph->numEdges; i++)
    {
        int j;

        for (j = 0; j < topoGraph->listOfEdges[i]->numNeighbors; j++)
        {
            if (topoGraph->listOfEdges[i]->neighbors[j] == edgeToDelete)
            {
                // "TGi <-- TGi - {(u,v)};"
                DeleteEdge(topoGraph, topoGraph->listOfEdges[i], j);
            }
        }
    }
}


static void
PruneTopologyTree(
    StarData *star)
{
    int i;
    // "for each ( link (u,v) in TGi | TGi(u,v).del = TRUE )"
    for (i = 0; i < star->topologyGraph_i.numEdges; i++)
    {
        if (star->topologyGraph_i.listOfEdges[i]->del == TRUE)
        {
            PruneEntryFromTopologyGraph(
                &(star->topologyGraph_i),
                star->topologyGraph_i.listOfEdges[i]);

        }
    }
}


static void
EliminateEdgesNotU_ToV(
    StarTopologyGraph *topoGraph,
    NodeAddress ipAddr_u,
    NodeAddress ipAddr_v)
{
    int i;

    for (i = 0; i < topoGraph->numEdges; i++)
    {
        // "if ( exists (r,s) in TGi(k) | r != u and s = v )"
        if (topoGraph->listOfEdges[i]->ipAddr_v == ipAddr_v &&
            topoGraph->listOfEdges[i]->ipAddr_u != ipAddr_u)
        {
            // "TGi(k) <-- TGi(k) - {(r,s)};"
#ifdef DEBUG
            printf("TGi(k) <-- TGi(k) - {(%d,%d)};\n",
                   topoGraph->listOfEdges[i]->ipAddr_u, ipAddr_v);
#endif
            PruneEntryFromTopologyGraph(
                topoGraph,
                topoGraph->listOfEdges[i]);
        }
    }
}


static void
ProcessVoidUpdate(
    StarData *star,
    NodeAddress k,
    StarLSU *lsu)
{
    // "Update topology graphs TGi and TGi(k) from LSU (u,v,l,t)
    //  reporting link failure."

    StarTopologyGraphEdgeEntry *edgeEntry;

#ifdef DEBUG
    printf("#%d: ProcessVoidUpdate(k = %d, u = %d, v = %d)\n",
           star->ipAddr_i, k, lsu->ipAddr_u, lsu->ipAddr_v);
#endif

    edgeEntry = ReturnTopologyGraphEdgeChildEntry(
                    &(star->topologyGraph_i),
                    lsu->ipAddr_u,
                    lsu->ipAddr_v);

    // "if ( (u,v) in TGi )"
    if (edgeEntry)
    {
        // "if ( t > TGi(u,v).t )"
        if (lsu->timestamp_t > edgeEntry->timestamp_t)
        {
            // "TGi(u,v).l <-- l; TGi(u,v).t <-- t;"
            edgeEntry->link_cost_l = lsu->link_cost_l;
            edgeEntry->timestamp_t = lsu->timestamp_t;
        }

        // "if ( k != i and (u,v) in TG(i)k )"
        if (k != star->ipAddr_i)
        {
            StarTopologyGraph *topoGraph_k;
            StarTopologyGraphEdgeEntry *TGik_edgeEntry;

            topoGraph_k = ReturnTopologyGraph(star, k);
            TGik_edgeEntry = ReturnTopologyGraphEdgeChildEntry(
                                 topoGraph_k,
                                 lsu->ipAddr_u,
                                 lsu->ipAddr_v);

            if (TGik_edgeEntry)
            {
                // "TG(i)k[u,v].l <-- l; TG(i)k[u,v].t <-- t;"
                TGik_edgeEntry->link_cost_l = lsu->link_cost_l;
                TGik_edgeEntry->timestamp_t = lsu->timestamp_t;
            }
        }

        // "TGi(u,v).del <-- FALSE;"
        edgeEntry->del = FALSE;
    }
}


static void
ProcessAddUpdate(
    StarData *star,
    NodeAddress k,
    StarLSU *lsu,
    Node *node)
{
    // "Update topology graphs TGi and TGi(k) from LSU (u,v,l,t)"

    StarTopologyGraphEdgeEntry *edgeEntry;

#ifdef DEBUG
    printf("#%d: ProcessAddUpdate(k = %d, u = %d, v = %d)\n",
           star->ipAddr_i, k, lsu->ipAddr_u, lsu->ipAddr_v);
#endif

    // The following pseudocode from the source document can be modified
    //     to eliminate the extra "if" clause without changing the
    //     algorithm at all.

    // "if ((u,v) not in TGi or t > TGi(u,v).t)"
    // "{"
    // "    if ((u,v) not in TGi)"
    // "        TGi <-- TGi U {(u,v,l,t)};"
    // "    else"
    // "    {"
    // "        TGi(u,v).l <-- l; TGi(u,v).t <-- t;"
    // "    }"
    // "}"

    // The version I am using is:

    //    if (u,v) not in TGi
    //        TGi <-- TGi U {(u,v,l,t)};
    //    else if (t > TGi(u,v).t)
    //        TGi(u,v).l <-- l; TGi(u,v).t <-- t;

    edgeEntry = ReturnTopologyGraphEdgeChildEntry(
                    &(star->topologyGraph_i),
                    lsu->ipAddr_u,
                    lsu->ipAddr_v);

    if (!edgeEntry)
    {
        // "TGi <-- TGi U {(u,v,l,t)};" from above should occur here

#ifdef DEBUG
        printf("\tTGi <-- TGi U {(%d,%d,%d,%d)\n",
               lsu->ipAddr_u,
               lsu->ipAddr_v,
               lsu->link_cost_l,
               lsu->timestamp_t);
#endif
        edgeEntry = AddEdgeToTopologyGraph(
                        &(star->topologyGraph_i),
                        /* k, */
                        lsu->ipAddr_u,
                        lsu->ipAddr_v,
                        lsu->link_cost_l,
                        lsu->timestamp_t,
                        node);
    }
    else
    if (lsu->timestamp_t > edgeEntry->timestamp_t)
    {
        // "TGi(u,v).l <-- l; TGi(u,v).t <-- t;" from above should occur here
        edgeEntry->link_cost_l = lsu->link_cost_l;
        edgeEntry->timestamp_t = lsu->timestamp_t;
    }

    // "if (k != i)"
    if (k != star->ipAddr_i)
    {
        StarTopologyGraph *topoGraph_k = ReturnTopologyGraph(star, k);
        StarTopologyGraphEdgeEntry *edgeEntry_uv;

        assert(topoGraph_k);

        // "if ( exists (r,s) in TGi(k) | r != u and s = v )"
        EliminateEdgesNotU_ToV(
            topoGraph_k,
            lsu->ipAddr_u,
            lsu->ipAddr_v);

        edgeEntry_uv = ReturnTopologyGraphEdgeChildEntry(
                           topoGraph_k,
                           lsu->ipAddr_u,
                           lsu->ipAddr_v);

        // "if ( (u,v) not in TGi(k) )"
        if (!edgeEntry_uv)
        {
            // "TGi(k) <-- TGi(k) U {(u, v, l, t)};"
            edgeEntry_uv = AddEdgeToTopologyGraph(
                               topoGraph_k,
                               /* k, */
                               lsu->ipAddr_u,
                               lsu->ipAddr_v,
                               lsu->link_cost_l,
                               lsu->timestamp_t,
                               node);

        }
        else
        {
            // "TGi(k).l <-- l; TGi(k).t <-- t;"
            edgeEntry_uv->link_cost_l = lsu->link_cost_l;
            edgeEntry_uv->timestamp_t = lsu->timestamp_t;
        }

        // "TGi(u,v).del <-- FALSE;"
        if (edgeEntry)
        {
            edgeEntry->del = FALSE;
        }
    }
}


static void
UpdateTopologyGraph(
    StarData *star,
    NodeAddress k,
    StarLSU *lsuPtr,
    int numLsu,
    Node *node)
{
    int i;

    // "Update TGi and TGi(k) from LSUs in msg"

    // "for each (LSU (u,v,l,t) in msg)"
    for (i = 0; i < numLsu; i++)
    {
        // "if (l != INFINITY_COST)"
        if (lsuPtr[i].link_cost_l != INFINITY_COST)
        {
            // "ProcessAddUpdate(k, (u,v,l,t));"
            ProcessAddUpdate(star, k, &(lsuPtr[i]), node);
        }
        else
        {
            // "ProcessVoidUpdate(k, (u,v,l,t));"
            ProcessVoidUpdate(star, k, &(lsuPtr[i]));
        }
    }
}


static void
InitializeSingleSource(
    StarTopologyGraph *topologyGraph,
    NodeAddress k,
    Node *node)
{
    int i;
    StarTopologyGraphVertexEntry *vertexEntry_k;

    // "for each (vertex v in TGi(k))"

    for (i = 0; i < topologyGraph->numVertexEntries; i++)
    {
        StarTopologyGraphVertexEntry *vertex;

        vertex = topologyGraph->vertexEntries[i];

        // "TGi(k)[v].d <-- INFINITY_COST;"
        vertex->distance_d = INFINITY_COST;

        // "TGi(k)[v].pred <-- NULL;"
        vertex->ipAddr_pred = (NodeAddress) NULL;

        // "TGi(k)[v].suc' <-- TGi(k)[v].suc'"
        vertex->ipAddr_suc_prime = vertex->ipAddr_suc;

        // "TGi(k)[v].suc <-- NULL;"
        vertex->ipAddr_suc = (NodeAddress) NULL;

        // "TGi(k)[v].nbr <-- NULL;"
        vertex->flag_nbr = (NodeAddress) NULL;

        vertex->extracted = FALSE;
    }

    // "TGi(k)[k].d <-- 0;"

    vertexEntry_k = ReturnTopologyGraphVertexEntry(topologyGraph, k);
    if (!vertexEntry_k)
    {
        AddVertexToTopologyGraph(
            topologyGraph,
            k,                    // ipAddr_vertex
            (NodeAddress) NULL,   // ipAddr_pred
            (NodeAddress) NULL,   // ipAddr_suc
            0,                    // distance_d
            (NodeAddress) NULL,   // flag_nbr
            node);
    }
    else
    {
        vertexEntry_k->distance_d = 0;
    }
}


static StarTopologyGraphVertexEntry *
ExtractMin(
    StarTopologyGraph *setOfVertices)
{
    int minimum = INFINITY_COST;
    StarTopologyGraphVertexEntry *result = NULL;
    int i;

    for (i = 0; i < setOfVertices->numVertexEntries; i++)
    {
        if ((setOfVertices->vertexEntries[i]->distance_d < minimum) &&
            (setOfVertices->vertexEntries[i]->valid == TRUE) &&
            (setOfVertices->vertexEntries[i]->extracted == FALSE))
        {
            minimum = setOfVertices->vertexEntries[i]->distance_d;
            result = setOfVertices->vertexEntries[i];
        }
    }

    if (result)
    {
#ifdef DEBUG
        printf("\tExtractMin returns %d\n", result->ipAddr_vertex);
#endif
        result->extracted = TRUE; // don't want to extract this one again
    }

    return result;
}


static void
Insert(
    StarTopologyGraph *setOfVertices,
    NodeAddress ipAddr_vertex,
    Node *node)
{
    int i;

#ifdef DEBUG
    printf("\tInsert(Q, %d)\n", ipAddr_vertex);
#endif

    for (i = 0; i < setOfVertices->numVertexEntries; i++)
    {
        if (setOfVertices->vertexEntries[i]->ipAddr_vertex == ipAddr_vertex)
        {
            setOfVertices->vertexEntries[i]->extracted = FALSE;
            return;
        }
    }

    AddVertexToTopologyGraph(
        setOfVertices,
        ipAddr_vertex,          // ipAddr_vertex
        (NodeAddress) NULL,     // ipAddr_pred
        (NodeAddress) NULL,     // ipAddr_suc
        INFINITY_COST,          // distance_d
        (NodeAddress) NULL,     // flag_nbr
        node);
}


static void
RelaxEdge(
    StarData *star,
    NodeAddress k,
    StarTopologyGraph* topologyGraph_i,
    StarTopologyGraph* topologyGraph_k,
    StarTopologyGraphVertexEntry *minimum_vertex_u,
    StarTopologyGraphEdgeEntry *edgeEntry_vertex_v,
    StarTopologyGraph *set_of_vertices_Q,
    NodeAddress ipAddr_suc,
    Node *node)
{
    int TGk_v_distance;
    int TGk_u_distance;
    int TGk_u_v_link_cost;
    StarTopologyGraphVertexEntry *TGk_vertex_v;

    TGk_vertex_v = ReturnTopologyGraphVertexEntry(
                       topologyGraph_k,
                       edgeEntry_vertex_v->ipAddr_v);

#ifdef DEBUG
    printf("\t#%d: RelaxEdge(k=%d, u=%d, v=%d, suc=%d)\n",
           star->ipAddr_i, k, minimum_vertex_u->ipAddr_vertex,
           edgeEntry_vertex_v->ipAddr_v, ipAddr_suc);
#endif

    if (TGk_vertex_v)
    {
        TGk_v_distance = TGk_vertex_v->distance_d;
    }
    else
    {
        TGk_v_distance = INFINITY_COST;

        TGk_vertex_v = AddVertexToTopologyGraph(
                           topologyGraph_k,
                           edgeEntry_vertex_v->ipAddr_v,  // ipAddr_vertex
                           (NodeAddress) NULL,            // ipAddr_pred
                           (NodeAddress) NULL,            // ipAddr_suc
                           INFINITY_COST,                 // distance_d
                           (NodeAddress) NULL,            // flag_nbr
                           node);
    }
    TGk_u_distance = minimum_vertex_u->distance_d;
    TGk_u_v_link_cost = edgeEntry_vertex_v->link_cost_l;

#ifdef DEBUG
    printf("\t\tTGk_v_dist = %d / TGk_u_dist = %d / TGi_u_v_link_cost = %d\n",
           TGk_v_distance, TGk_u_distance, TGk_u_v_link_cost);
#endif

    // "if ( TGi(k)[v].d > TGi(k)[u].d + TGi(k)[u,v].l or
    //       ( k = i and TGi(k)[v].d = TGi(k)[u].d + TGi(k)[u,v].l and
    //         (u,v) in STi ) )
    if ( (TGk_v_distance > TGk_u_distance + TGk_u_v_link_cost) ||
         ( (k == star->ipAddr_i) &&
           (TGk_v_distance == TGk_u_distance + TGk_u_v_link_cost) &&
           (ReturnTopologyGraphEdgeChildEntry(
                &(star->topoGraphST),
                minimum_vertex_u->ipAddr_vertex,
                edgeEntry_vertex_v->ipAddr_v)) ) )
    {
        StarTopologyGraphVertexEntry *TGi_vertex_v;
        BOOL TGi_vertex_v_suc_prime_is_null = FALSE;

        TGi_vertex_v = ReturnTopologyGraphVertexEntry(
                           topologyGraph_i,
                           edgeEntry_vertex_v->ipAddr_v);

        // "TGi(k)[v].d <-- TGi(k)[u].d + TGi(k)[u,v].l;"
        TGk_vertex_v->distance_d = TGk_u_distance + TGk_u_v_link_cost;

        // "TGi(k)[v].pred <-- TGi(k)[u,v];"
        TGk_vertex_v->ipAddr_pred = minimum_vertex_u->ipAddr_vertex;

        // "TGi(k)[v].suc <-- suc;"
        TGk_vertex_v->ipAddr_suc = ipAddr_suc;

        if (TGi_vertex_v)
        {
            if (TGi_vertex_v->ipAddr_suc_prime == (NodeAddress) NULL)
            {
                TGi_vertex_v_suc_prime_is_null = TRUE;
            }
        }
        else
        {
            TGi_vertex_v_suc_prime_is_null = TRUE;

            TGi_vertex_v = AddVertexToTopologyGraph(
                               topologyGraph_i,
                               edgeEntry_vertex_v->ipAddr_v,  // ipAddr_vertex
                               (NodeAddress) NULL,            // ipAddr_pred
                               (NodeAddress) NULL,            // ipAddr_suc
                               INFINITY_COST,                 // distance_d
                               (NodeAddress) NULL,            // flag_nbr
                               node);

        }

        // "if (LORA and k = i and TGi(v).suc' = null)"
        if ( (star->LORAmode == TRUE) &&
             (k == star->ipAddr_i) &&
             (TGi_vertex_v_suc_prime_is_null) )
        {
            // "v was an unknown destination"

            // "TGi(v).suc' <-- suc;"
            TGi_vertex_v->ipAddr_suc_prime = ipAddr_suc;

            // "TGi(v).d'' <-- TGi(v).d;"
            TGi_vertex_v->distance_d_double_prime = TGi_vertex_v->distance_d;

            // "if ( suc != i )"
            if (ipAddr_suc != star->ipAddr_i)
            {
                StarTopologyGraphEdgeEntry *edgeEntry_i_suc;

                edgeEntry_i_suc = ReturnTopologyGraphEdgeChildEntry(
                                      &(star->topologyGraph_i),
                                      star->ipAddr_i,
                                      ipAddr_suc);

                assert(edgeEntry_i_suc);
                // "TGi(v).d' <-- TGi(v).d - TGi(i,suc).l;"
                TGi_vertex_v->distance_d_prime = TGi_vertex_v->distance_d
                                           - edgeEntry_i_suc->link_cost_l;
            }
            else
            {
                // "TGi(v).d' <-- 0;"
                TGi_vertex_v->distance_d_prime = 0;
            }
        }

        // "Insert(Q, v);"
        Insert(set_of_vertices_Q,
               edgeEntry_vertex_v->ipAddr_v,
               node);
    }
}


static void
ReportNoDestinationToS(
    StarData *star,
    StarTopologyGraph *topoGraph_i,
    NodeAddress ipAddr_s)
{
    int i;

    // "for each ( link (r,s) in TGi | s = v )"
    for (i = 0; i < topoGraph_i->numEdges; i++)
    {
        if (ipAddr_s == topoGraph_i->listOfEdges[i]->ipAddr_v)
        {
           // "if ( TGi(r,s).l = INFINITY_COST )"
           // if (topoGraph_i->listOfEdges[i]->link_cost_l == INFINITY_COST)
           //{
           //     // "MSGi <-- MSGi U {(r,s,TGi(r,s).l,TGi(r,s).t)};"
           //     AddLSUToMSGi(
           //         star,
           //         topoGraph_i->listOfEdges[i]->ipAddr_u,
           //         topoGraph_i->listOfEdges[i]->ipAddr_v,
           //         topoGraph_i->listOfEdges[i]->link_cost_l,
           //         topoGraph_i->listOfEdges[i]->timestamp_t);
           // }
        }
    }
}


static void
UpdateNeighborTree(
    StarData *star,
    NodeAddress k,
    StarTopologyGraph *newST,
    StarTopologyGraph *shortestPathTree_k,
    StarTopologyGraph *topologyGraph_i,
    StarTopologyGraph *topologyGraph_k)
{
    // "Delete links from TGi(k) and report failed links"
    int i;

#ifdef DEBUG
    printf("\tUpdateNeighborTree(i = %d ; k = %d)\n",
           star->ipAddr_i, k);
#endif

    // "for each ( link (u,v) in STi(k) )"
    for (i = 1; i < shortestPathTree_k->numEdges; i++)
    {
        StarTopologyGraphEdgeEntry *STik_link_uv;

        STik_link_uv = shortestPathTree_k->listOfEdges[i];

        // "if ( (u,v) ! in newST )"
        if (!ReturnTopologyGraphEdgeChildEntry(
                 newST,
                 STik_link_uv->ipAddr_u,
                 STik_link_uv->ipAddr_v))
        {
            StarTopologyGraphVertexEntry *TGik_vertexEntry_v;
            StarTopologyGraphVertexEntry *TGi_vEntry_v;

            TGik_vertexEntry_v = ReturnTopologyGraphVertexEntry(
                                     topologyGraph_k,
                                     STik_link_uv->ipAddr_v);

            assert(TGik_vertexEntry_v);

            TGi_vEntry_v = ReturnTopologyGraphVertexEntry(
                               topologyGraph_i,
                               STik_link_uv->ipAddr_v);

            assert(TGi_vEntry_v);

#ifdef DEBUG
            printf("k has removed (%d, %d) from its source tree\n",
                   STik_link_uv->ipAddr_u, STik_link_uv->ipAddr_v);
#endif

            // "k has removed (u,v) from its source tree"

            // "if ( LORA and TGi(k)[v].pred = NULL )"
            if (star->LORAmode &&
                TGik_vertexEntry_v->ipAddr_pred == (NodeAddress) NULL)
            {
                // "LORA-2 rule: k has no path to destination v"
#ifdef DEBUG
                printf("LORA-2 rule: k%d has no path to destination v%d\n",
                       k, STik_link_uv->ipAddr_v);
#endif

                // "Mi <-- TRUE;"
                star->Mi = TRUE;

                // "if ( k = i )"
                if (k == star->ipAddr_i)
                {
                    // "for each ( link (r,s) in TGi | s = v )"
                    ReportNoDestinationToS(
                        star,
                        topologyGraph_i,
                        STik_link_uv->ipAddr_v);
                }
            }

            // "if ( ORA and k = i and (u = i or TGi(v).pred = null ) )"
            if ( (star->LORAmode == FALSE) &&
                 (k == star->ipAddr_i) &&
                 ( (STik_link_uv->ipAddr_u == star->ipAddr_i) ||
                   (TGi_vEntry_v->ipAddr_pred == (NodeAddress) NULL) ) )
            {
                // "i has no path to destination v or i is the head node"
#ifdef DEBUG
                printf("i has no path to destination v or "
                       "i is the head node\n");
#endif

                // "if ( TGi(v).pred = null )"
                if (TGi_vEntry_v->ipAddr_pred == (NodeAddress) NULL)
                {
                    // "for each ( link (r,s) in TGi | s = v )"
                    ReportNoDestinationToS(
                        star,
                        topologyGraph_i,
                        STik_link_uv->ipAddr_v);
                }
                else
                {
                    StarTopologyGraphEdgeEntry *TGi_edgeEntry_uv;

                    TGi_edgeEntry_uv = ReturnTopologyGraphEdgeChildEntry(
                                           topologyGraph_i,
                                           STik_link_uv->ipAddr_u,
                                           STik_link_uv->ipAddr_v);

                   // assert(TGi_edgeEntry_uv);
                   //
                   // // "if ( TGi(u,v).l = INFINITY_COST )"
                   // if (TGi_edgeEntry_uv->link_cost_l == INFINITY_COST)
                   // {
                   //  // "i needs to report failed link"
                   //  // "MSGi <-- MSGi U {(u, v, TGi(u,v).l, TGi(u,v).t)};"
                   //     AddLSUToMSGi(
                   //         star,
                   //         STik_link_uv->ipAddr_u,
                   //         STik_link_uv->ipAddr_v,
                   //         TGi_edgeEntry_uv->link_cost_l,
                   //         TGi_edgeEntry_uv->timestamp_t);
                   //}
                }
            }
            // "if ( LORA and k = i and TGi(v).pred = null )"
            if (star->LORAmode &&
                k == star->ipAddr_i &&
                TGi_vEntry_v->ipAddr_pred == (NodeAddress) NULL)
            {
                // "TGi(v).d' <-- INFINITY_COST;
                TGi_vEntry_v->distance_d_prime = INFINITY_COST;

                // "TGi(v).suc' <-- NULL;"
                TGi_vEntry_v->ipAddr_suc = (NodeAddress) NULL;
            }
            // "if ( NOT ( k == i and u == i ) )"
            if (k != star->ipAddr_i ||
                STik_link_uv->ipAddr_u != star->ipAddr_i)
            {
                StarTopologyGraphEdgeEntry *TGi_edgeEntry_uv;

                StarTopologyGraphEdgeEntry *TGk_edgeEntry;

                TGi_edgeEntry_uv = ReturnTopologyGraphEdgeChildEntry(
                                       topologyGraph_i,
                                       STik_link_uv->ipAddr_u,
                                       STik_link_uv->ipAddr_v);


                TGk_edgeEntry = ReturnTopologyGraphEdgeChildEntry(
                                    topologyGraph_k,
                                    STik_link_uv->ipAddr_u,
                                    STik_link_uv->ipAddr_v);


       // "if ( (u,v) in TGi(k) )
    if (TGk_edgeEntry)
    {
         int i;

         StarTopologyGraphEdgeEntry *parentEntry;

         parentEntry = ReturnTopologyGraphEdgeParentEntry(
                                      topologyGraph_k,
                                      STik_link_uv->ipAddr_u);

         // "TGi(k) <-- TGi(k) - {(u,v)};"
         for (i = 0; parentEntry != NULL && i < parentEntry->numNeighbors; i++)
         {
             if (parentEntry->neighbors[i]->ipAddr_v
                      == STik_link_uv->ipAddr_v)
             {
                  DeleteEdge(topologyGraph_k, parentEntry, i);
             }
         }
    }
                // " if ( TGi(u,v).l != INFINITY_COST and not exists x in Ni"
                // " | (u,v) in TGi(x) )"
                if (TGi_edgeEntry_uv != NULL)
                {
                    int i;
                    BOOL edgeExists = FALSE;

                    for (i = 0; i < NUM_NEIGHBORS(star); i++)
                    {
                        NodeAddress neighbor;
                        StarTopologyGraph *topoGraph;

                        neighbor = RETURN_NEIGHBOR_ADDRESS(star, i);
                        topoGraph =  ReturnTopologyGraph(star, neighbor);

                        if (ReturnTopologyGraphEdgeChildEntry(
                                topoGraph,
                                STik_link_uv->ipAddr_u,
                                STik_link_uv->ipAddr_v))
                        {
                            edgeExists = TRUE;
                            break;
                        }
                    }

                    if (!edgeExists)
                    {
                        // " TGi(u,v).del = TRUE;"
                        TGi_edgeEntry_uv->del = TRUE;
                    }
                }
            }
        }
    }
}


static void
ReportChanges(
    StarData *star,
    StarTopologyGraph *oldST,
    StarTopologyGraph *newST)
{
    // "Generate LSUs for new links in the router's source tree"
    int i;

    // "for each ( link (u,v) in newST )"
    // "    if (u,v) not in oldST or newST(u,v).t != oldST(u,v).t or NSi )"
    // "        MSGi <-- MSGi U { (u, v, TGi(u,v).l, TGi(u,v).t) };"
    for (i = 1; i < newST->numEdges; i++)
    {
        StarTopologyGraphEdgeEntry *oldST_edgeEntry;

        oldST_edgeEntry = ReturnTopologyGraphEdgeChildEntry(
                              oldST,
                              newST->listOfEdges[i]->ipAddr_u,
                              newST->listOfEdges[i]->ipAddr_v);

        // "if (u,v) not in oldST or newST(u,v).t != oldST(u,v).t or NSi )"
        // "    MSGi <-- MSGi U { (u, v, TGi(u,v).l, TGi(u,v).t) };"
        if ( (!oldST_edgeEntry) ||
             (oldST_edgeEntry->timestamp_t
              != newST->listOfEdges[i]->timestamp_t) ||
             (star->NSi) )
        {
            StarTopologyGraphEdgeEntry *TGi_edgeEntry;

            TGi_edgeEntry = ReturnTopologyGraphEdgeChildEntry(
                                &(star->topologyGraph_i),
                                newST->listOfEdges[i]->ipAddr_u,
                                newST->listOfEdges[i]->ipAddr_v);

            //...
            //assert(TGi_edgeEntry);

            //AddLSUToMSGi(
            //    star,
            //    newST->listOfEdges[i]->ipAddr_u,
            //    newST->listOfEdges[i]->ipAddr_v,
            //    TGi_edgeEntry->link_cost_l,
            //   TGi_edgeEntry->timestamp_t);
            //...
        }
    }
}



static void
BuildShortestPathTree(
    StarData *star,
    NodeAddress k,
    Node *node)
{
    // "Construct STi(k)"

    StarTopologyGraph* topologyGraph_k;
    StarTopologyGraph *set_of_vertices_Q;
    StarTopologyGraphVertexEntry *min_vertex_u;
    StarTopologyGraph newST;
    StarTopologyGraphEdgeEntry *vertex_u_edgeEntry;
    StarTopologyGraph* topologyGraph_i;
    StarTopologyGraph* shortestPathTree_k;

    topologyGraph_i = ReturnTopologyGraph(star, star->ipAddr_i);

    topologyGraph_k = ReturnTopologyGraph(star, k);

    assert(topologyGraph_i);
    assert(topologyGraph_k);

#ifdef DEBUG
    printf("#%d: BuildShortestPathTree(k = %d)\n", star->ipAddr_i, k);
#endif

    // "InitializeSingleSource(k);"
    InitializeSingleSource(topologyGraph_k, k, node);

    // "Q <-- set of vertices in TG(i)k"
    set_of_vertices_Q = topologyGraph_k;

    // "u <-- ExtractMin(Q);
    min_vertex_u = ExtractMin(set_of_vertices_Q);

    // "newST <-- NULL;"
    InitializeTopologyGraph(&newST, k, node);

    // "while ( u != NULL and TGi(k)[u].d < INFINITY_COST )"
    while ((min_vertex_u != NULL) &&
           (min_vertex_u->distance_d < INFINITY_COST))
    {
        int i;

        vertex_u_edgeEntry = ReturnTopologyGraphEdgeParentEntry(
                                 topologyGraph_k,
                                 min_vertex_u->ipAddr_vertex);
#ifdef DEBUG
        printf("\tvertex_u_edgeEntry - %d - %d / linkcost %d - %d neighbors\n",
               vertex_u_edgeEntry->ipAddr_u,
               vertex_u_edgeEntry->ipAddr_v,
               vertex_u_edgeEntry->link_cost_l,
               vertex_u_edgeEntry->numNeighbors);

        printf("\tu = %d TGi(k)[u].pred = %d suc = %d distance_d = %d\n",
               min_vertex_u->ipAddr_vertex,
               min_vertex_u->ipAddr_pred,
               min_vertex_u->ipAddr_suc,
               min_vertex_u->distance_d);
#endif

        // "if ( TGi(k)[u].pred != NULL and TGi(k)[u].pred not in newST)"
        if ((min_vertex_u->ipAddr_pred != (NodeAddress) NULL) &&
            (!ReturnTopologyGraphEdgeChildEntry(
                  &newST,
                  min_vertex_u->ipAddr_pred,
                  min_vertex_u->ipAddr_vertex)))
        {
            // "(r,s) <-- TGi(k)[u].pred;"
            // "newST <-- newST U (r,s);"
            AddEdgeToTopologyGraph(
                &newST,
                min_vertex_u->ipAddr_pred,
                min_vertex_u->ipAddr_vertex,
                min_vertex_u->distance_d,
                vertex_u_edgeEntry->timestamp_t,
                node);

            // "if ( LORA and k = i )"
            if (star->LORAmode && k == star->ipAddr_i)
            {
                NodeAddress ipAddr_w;
                int path_w_u_cost;

                // "if ( i > TGi(u).suc )"
                if (star->ipAddr_i > min_vertex_u->ipAddr_suc)
                {
                    StarTopologyGraph *topologyGraph_x;

                    topologyGraph_x = ReturnTopologyGraph(
                                          star,
                                          min_vertex_u->ipAddr_suc);

                    // "if ( exists x in Ni"
                    // "     | TGi(x)[u].suc = i and TGi(u).suc = x )"
                    if (topologyGraph_x) // x in Ni with a TGi(x)
                    {
                        StarTopologyGraphVertexEntry *TGix_vertex_u =
                            ReturnTopologyGraphVertexEntry(
                                topologyGraph_x,
                                min_vertex_u->ipAddr_vertex);

                        if ((TGix_vertex_u) &&
                            (TGix_vertex_u->ipAddr_suc == star->ipAddr_i))
                        {
                            // "Mi <-- TRUE;   // LORA-3 rule"
                            star->Mi = TRUE;
#ifdef DEBUG
                            printf("\t\tLORA-3 rule (1)\n");
#endif
                        }
                    }
                }

                // "if ( TGi(u).suc != TGi(u).suc' and TGi(u).suc > i )"
                if ( (min_vertex_u->ipAddr_suc !=
                      min_vertex_u->ipAddr_suc_prime) &&
                     (min_vertex_u->ipAddr_suc > star->ipAddr_i) )
                {
                    // "Mi <-- TRUE;   // LORA-3 rule"
                    star->Mi = TRUE;
#ifdef DEBUG
                    printf("\t\tLORA-3 rule (2)\n");
#endif
                }

                // "if ( not exists (x,y) in STi' | y = u )"
                if (!ReturnTopologyGraphEdgeParentEntry(
                          &(star->topoGraphST_prime),
                          min_vertex_u->ipAddr_vertex))
                {
                    // "Mi <-- TRUE;   // LORA-1 rule"
                    star->Mi = TRUE;
#ifdef DEBUG
                    printf("\t\tLORA-1 rule\n");
#endif
                }

                // "if ( TGi(u).d'' != INFINITY_COST and
                // "     abs(TGi(u).d - TGi(u).d'') > delta )
               if ( (min_vertex_u->distance_d_double_prime != INFINITY_COST) &&
                     (abs(min_vertex_u->distance_d -
                          min_vertex_u->distance_d_prime) > star->delta) )
               {
                    // "Mi <-- TRUE;   // LORA-2 rule"
                    star->Mi = TRUE;
#ifdef DEBUG
                    printf("\t\tLORA-2 rule\n");
#endif
               }

                // "w <-- TGi(u).suc;"
                ipAddr_w = min_vertex_u->ipAddr_suc;

                // "if (w != i)"
                if (ipAddr_w != star->ipAddr_i)
                {
                    StarTopologyGraphEdgeEntry *edgeEntry_w =
                        ReturnTopologyGraphEdgeChildEntry(
                            &(star->topologyGraph_i),
                            star->ipAddr_i,
                            ipAddr_w);

                    assert(edgeEntry_w);
                    // "path.w.u.cost <-- TGi(u).d - TGi(i,w).l;
                    path_w_u_cost = min_vertex_u->distance_d
                                    - edgeEntry_w->link_cost_l;
                }
                else
                {
                    // "path.w.u.cost <-- 0;"
                    path_w_u_cost = 0;
                }

                // "if ( path_w_u_cost > TGi(u).d' )"
                if (path_w_u_cost > min_vertex_u->distance_d_prime)
                {
                    StarTopologyGraphVertexEntry *vertexEntry_r;

                    StarTopologyGraphVertexEntry *vertexEntry_s;

                    vertexEntry_r = ReturnTopologyGraphVertexEntry(
                                        &(star->topologyGraph_i),
                                        min_vertex_u->ipAddr_pred);

                    vertexEntry_s = ReturnTopologyGraphVertexEntry(
                                    &(star->topologyGraph_i),
                                    min_vertex_u->ipAddr_pred);

                    assert(vertexEntry_r);

                    assert(vertexEntry_s);

                    // "if ( r = w or TGi(r).nbr = i )"
                    if ( (min_vertex_u->ipAddr_pred == ipAddr_w) ||
                         (vertexEntry_r->flag_nbr == star->ipAddr_i) )
                    {
                        // "TGi(s).nbr <-- i;"
                        vertexEntry_s->flag_nbr = star->ipAddr_i;
                    }
                    // "if ( TGi(s).nbr != i )"
                    if (vertexEntry_s->flag_nbr != star->ipAddr_i)
                    {
                        // "Mi <-- TRUE; // LORA-3 rule"
                        star->Mi = TRUE;
#ifdef DEBUG
                        printf("\t\tLORA-3 rule (3)\n");
#endif
                    }
                }

                // "TGi(u).d' <-- path.w.u.cost;"
                min_vertex_u->distance_d_prime = path_w_u_cost;

                // "TGi(u).suc' <-- TGi(u).suc;"
                min_vertex_u->ipAddr_suc_prime = min_vertex_u->ipAddr_suc;
            }
        }

        // "for each ( vertex v in adjacency list of TGi(k)[u] "
        // " | TGi(k)[u,v].l != INFINITY_COST and NOT TGi(u,v).del))"

        for (i = 0; i < vertex_u_edgeEntry->numNeighbors &&
             vertex_u_edgeEntry->neighbors[i] != NULL; i++)
        {
            NodeAddress ipAddr_suc;
            StarTopologyGraphEdgeEntry *sourceGraphEdgeEntry =
                ReturnTopologyGraphEdgeChildEntry(
                    topologyGraph_k,
                    min_vertex_u->ipAddr_vertex,
                    vertex_u_edgeEntry->neighbors[i]->ipAddr_v);

            // "TGi(k)[u,v].l != INFINITY_COST and NOT TGi(u,v).del"
            // I think this should check whether or not v == i
            if ((sourceGraphEdgeEntry) &&
                (sourceGraphEdgeEntry->ipAddr_v != star->ipAddr_i) &&
                (sourceGraphEdgeEntry != NULL) &&
                (sourceGraphEdgeEntry->del != TRUE))
            {
                // "if ( k = i )"
                if (k == star->ipAddr_i)
                {
                    // "if ( u = i )"
                    if (min_vertex_u->ipAddr_vertex == star->ipAddr_i)
                    {
                        // "suc = i;"
                        ipAddr_suc = star->ipAddr_i;
                    }
                    else // "else if ( TGi(u).suc = i )"
                    {
                        StarTopologyGraphVertexEntry *sourceVertex =
                            ReturnTopologyGraphVertexEntry(
                                topologyGraph_i,
                                min_vertex_u->ipAddr_vertex);

                        assert(sourceVertex);
                        if (sourceVertex->ipAddr_suc == star->ipAddr_i)
                        {
                            // "suc <-- {x | x in Ni and x = u"
                            ipAddr_suc = min_vertex_u->ipAddr_vertex;

                            assert(ReturnNeighborEdgeEntry(
                                       star->topologyGraph_i.edgeRoot,
                                       min_vertex_u->ipAddr_vertex));
                        }
                        else
                        {
                            // "suc <-- TGi(u).suc"
                            ipAddr_suc = sourceVertex->ipAddr_suc;
                        }
                    }
                }
                else
                {
                    // "if ( u = k )"
                    if (min_vertex_u->ipAddr_vertex == k)
                    {
                        // "if ( v = i )"
                        if ( vertex_u_edgeEntry->neighbors[i]->ipAddr_v
                             == star->ipAddr_i)
                        {
                            // "suc <-- i;"
                            ipAddr_suc = star->ipAddr_i;
                        }
                        else
                        {
                            // "suc <-- k;"
                            ipAddr_suc = k;
                        }
                    }
                    else
                    {
                        StarTopologyGraphVertexEntry *sourceVertex =
                            ReturnTopologyGraphVertexEntry(
                                topologyGraph_i,
                                min_vertex_u->ipAddr_vertex);

                        assert(sourceVertex);
                        // "suc <-- TGi(u).suc;"
                        ipAddr_suc = sourceVertex->ipAddr_suc;
                    }
                }

                // "RelaxEdge(k, u, v, Q, suc);"
                RelaxEdge(
                    star,
                    k,
                    topologyGraph_i,
                    topologyGraph_k,
                    min_vertex_u,
                    vertex_u_edgeEntry->neighbors[i],
                    set_of_vertices_Q,
                    ipAddr_suc,
                    node);
            }
        }

        // "if ( Q != 0 ) u <-- ExtractMin(Q);"
        // "else          u <-- null;"
        min_vertex_u = ExtractMin(set_of_vertices_Q);
    }

#ifdef DEBUG
    PrintTopologyGraph(star->ipAddr_i, &newST);
#endif

    shortestPathTree_k = ReturnShortestPathTree(star, k);
    assert(shortestPathTree_k);

    // "UpdateNeighborTree(k, newST);"
    UpdateNeighborTree(
        star,
        k,
        &newST,
        shortestPathTree_k,
        topologyGraph_i,
        topologyGraph_k);

    // "if ( k = i )"
    if (k == star->ipAddr_i)
    {
        // "if ( LORA and Mi )"
        if (star->LORAmode && star->Mi)
        {
            // "ReportChanges(STi', newST);"

            ReportChanges(
                star,
                &(star->topoGraphST_prime),
                &newST);

           // "STi' <-- newST; NSi <-- FALSE;"
            ReplaceShortestPathTree(&(star->topoGraphST_prime), &newST, node);
            star->NSi = FALSE;
        }
        // "else if ( ORA )"
        else
        if (star->LORAmode == FALSE)
        {
            // "ReportChanges(STi', newST);"
            ReportChanges(
                star,
                &(star->topoGraphST_prime),
                &newST);

            // I think that we should also assign newST to STi'
            ReplaceShortestPathTree(&(star->topoGraphST_prime), &newST, node);
        }

        // "if ( ORA or ( LORA and Mi ) )"
        if ( (star->LORAmode == FALSE) ||
             (star->LORAmode && star->Mi) )
        {
            // "for each ( link (u,v) in TGi | TGi(u,v).del = TRUE )"
            // "    TGi <-- TGi - {(u,v)};"
            PruneTopologyTree(star);
        }

        // "Mi <-- FALSE;"
        star->Mi = FALSE;
    }

    // "STi(k) <-- newST; newST <-- 0;"
    ReplaceShortestPathTree(shortestPathTree_k, &newST, node);

    EmptyTopologyGraph(&newST, node);
}


static void
Send(
    Node *node,
    StarData *star)
{
    Message *newMsg;
    int index = 0;
    clocktype cumulativeJitter = 0;
    clocktype packetJitter = (clocktype) (RANDOM_erand(star->seed) * STAR_JITTER);

    star->stats.numLSU_sent += star->MSGi_numEntries;

    while (star->MSGi_numEntries > 0)
    {
        int numLSUs = MIN(MAX_LSUS_IN_PACKET, star->MSGi_numEntries);
#ifdef DEBUG
        printf("#%d: Send(%d LSUs)\n",
               star->ipAddr_i, numLSUs);
#endif
        cumulativeJitter += packetJitter;

        newMsg = MESSAGE_Alloc(node, MAC_LAYER, 0, MSG_MAC_FromNetwork);
        MESSAGE_PacketAlloc(
            node,
            newMsg,
            sizeof(StarLSU) * numLSUs,
            TRACE_STAR);

        memcpy(
            MESSAGE_ReturnPacket(newMsg),
            &(star->MSGi[index]),
            sizeof(StarLSU) * numLSUs);

        NetworkIpSendRawMessageWithDelay(
                node,
                newMsg,
                ANY_IP,
                ANY_DEST,
                ANY_INTERFACE,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_STAR,
                0,
                cumulativeJitter);

        packetJitter = (clocktype) (RANDOM_erand(star->seed) * STAR_JITTER);

        star->MSGi_numEntries -= numLSUs;
        star->stats.numPkts_sent++;
        index += numLSUs;
    }
}


static void
UpdateRoutingTable(
    Node *node,
    StarData *star)
{
    int i;

    for (i = 0; i < star->topologyGraph_i.numVertexEntries; i++)
    {
#ifdef DEBUG
        printf("dest %d - suc = %d - distance = %d\n",
               star->topologyGraph_i.vertexEntries[i]->ipAddr_vertex,
               star->topologyGraph_i.vertexEntries[i]->ipAddr_suc,
               star->topologyGraph_i.vertexEntries[i]->distance_d);
#endif

    if (star->topologyGraph_i.vertexEntries[i]->ipAddr_vertex !=
            star->ipAddr_i)
    {
        if (star->topologyGraph_i.vertexEntries[i]->distance_d < INFINITY_COST)
        {
                if (star->topologyGraph_i.vertexEntries[i]->ipAddr_suc ==
                    star->ipAddr_i)
                {
                    NetworkUpdateForwardingTable(
                        node,
                        star->topologyGraph_i.vertexEntries[i]->ipAddr_vertex,
                        ANY_DEST,
                        star->topologyGraph_i.vertexEntries[i]->ipAddr_vertex,
                        ANY_INTERFACE,
                        0,
                        ROUTING_PROTOCOL_STAR);
                }
                else
                {
                    NetworkUpdateForwardingTable(
                        node,
                        star->topologyGraph_i.vertexEntries[i]->ipAddr_vertex,
                        ANY_DEST,
                        star->topologyGraph_i.vertexEntries[i]->ipAddr_suc,
                        ANY_INTERFACE,
                        0,
                        ROUTING_PROTOCOL_STAR);
                }
        }
        else
        {
                NetworkUpdateForwardingTable(
                    node,
                    star->topologyGraph_i.vertexEntries[i]->ipAddr_vertex,
                    ANY_DEST,
                    (unsigned) NETWORK_UNREACHABLE,
                    ANY_INTERFACE,
                    -1,
                    ROUTING_PROTOCOL_STAR);
            }
        }
    }
}


static void
Update(
    Node *node,
    StarData *star,
    NodeAddress k,
    StarLSU *lsuPtr,
    int numLsu)
{
    // "Process update message sent by router k"

#ifdef DEBUG
    printf("     #%d: Update()", star->ipAddr_i);
#endif

    // "UpdateTopologyGraph(k, msg);"
    UpdateTopologyGraph(star, k, lsuPtr, numLsu, node);

    // "if (k != i)"
    if (k != star->ipAddr_i)
    {
        // "BuildShortestPathTree(k);"
        BuildShortestPathTree(star, k, node);
    }

    // "BuildShortestPathTree(i);"
    BuildShortestPathTree(star, star->ipAddr_i, node);

    // "UpdateRoutingTable();"
    UpdateRoutingTable(node, star);

    // "if ( k != i )"
    if (k != star->ipAddr_i)
    {
        // "Send();"
        Send(node, star);
    }
}


static void
AddShortestPathTreeLinksToMSG(
    StarData *star,
    StarTopologyGraph *shortestPathTree,
    Node *node)
{
    int i;

    // for each (link (u,v)) in the given shortest path tree ST
    for (i = 1; i < shortestPathTree->numEdges; i++)
    {
        // MSGi <-- MSGi U {(u,v,ST(u,v).l,ST(u,v).t)};"
        AddLSUToMSGi(
            star,
            shortestPathTree->listOfEdges[i]->ipAddr_u,
            shortestPathTree->listOfEdges[i]->ipAddr_v,
            shortestPathTree->listOfEdges[i]->link_cost_l,
            shortestPathTree->listOfEdges[i]->timestamp_t);
    }
}


static void
GenericAddTopologyGraphToSomeList(
    StarTopologyGraph **topoGraph,
    int *numEntries,
    int *maxEntries,
    NodeAddress ipAddr_k,
    Node *node)
{
    int index = *numEntries;

    if (index == *maxEntries)
    {
        StarTopologyGraph *nextTopoGraph;

        nextTopoGraph =
            (StarTopologyGraph *)
            MEM_malloc(sizeof(StarTopologyGraph)
                       * (index + INITIAL_NUM_ENTRIES));


        memcpy(nextTopoGraph, *topoGraph, sizeof(StarTopologyGraph) * index);

        MEM_free(*topoGraph);

        *topoGraph = nextTopoGraph;
        *maxEntries = index + INITIAL_NUM_ENTRIES;
    }


    // Need not add duplicate entries...
    for (int i = 0; i < *numEntries; i++)
    {
        if (((*topoGraph)[i]).edgeRoot->ipAddr_u == ipAddr_k &&
            ((*topoGraph)[i]).edgeRoot->ipAddr_v == ipAddr_k)
        {
            int j = 0;
            for (j = 0 ;j < (*topoGraph)[i].numEdges ; j++)
            {
                 MEM_free((*topoGraph)[i].listOfEdges[j]->neighbors);
                 MEM_free((*topoGraph)[i].listOfEdges[j]);
            }

            for (j = 0; j < ((*topoGraph)[i]).numVertexEntries; j++)
            {
               MEM_free(((*topoGraph)[i]).vertexEntries[j]);
            }

            MEM_free(((*topoGraph)[i]).vertexEntries);
            MEM_free(((*topoGraph)[i]).listOfEdges);

            InitializeTopologyGraph(&((*topoGraph)[i]), ipAddr_k, node);
            return;

        }
    }



    InitializeTopologyGraph(&((*topoGraph)[index]), ipAddr_k, node);

    *numEntries = index + 1;
}



static void
AddTopologyGraph(
    StarData *star,
    NodeAddress ipAddr_k,
    Node *node)
{
    GenericAddTopologyGraphToSomeList(
        &(star->topoGraph_k),
        &(star->numTopoGraph_k),
        &(star->maxTopoGraph_k),
        ipAddr_k,
        node);
}



static void
AddShortestPathTree(
    StarData *star,
    NodeAddress ipAddr_k,
    Node *node)
{
    GenericAddTopologyGraphToSomeList(
        &(star->ShortestPathTree_k),
        &(star->numST_k),
        &(star->maxST_k),
        ipAddr_k,
        node);
}



static void
NeighborDown(
    Node *node,
    StarData *star,
    NodeAddress k)
{
    // "Neighbor protocol reports link failure to neighbor k"

    StarTopologyGraph *topoGraph_k;
    StarTopologyGraph *STik;
    StarLSU lsu;
    int i;

    topoGraph_k = ReturnTopologyGraph(star, k);
    STik = ReturnShortestPathTree(star, k);

    // "Ni <-- Ni - { k };"
    for (i = 0; i < star->topologyGraph_i.edgeRoot->numNeighbors; i++)
    {
        if (star->topologyGraph_i.edgeRoot->neighbors[i]->ipAddr_v == k)
        {
           DeleteEdge(
                &(star->topologyGraph_i),
                star->topologyGraph_i.edgeRoot,
                i);
        }
    }

    // "TGi(k) <-- 0;"
    EmptyTopologyGraph(topoGraph_k, node);
    InitializeTopologyGraph(topoGraph_k, k, node);

    // "STi(k) <-- 0;"
    EmptyTopologyGraph(STik, node);
    InitializeTopologyGraph(STik, k, node);

    // "Update(i, (i,k,INFINITY_COST, Ti));"
    lsu.ipAddr_u = star->ipAddr_i;
    lsu.ipAddr_v = k;
    lsu.link_cost_l = INFINITY_COST;
    lsu.timestamp_t = STAR_CURRENT_CLOCK();
    Update(node, star, star->ipAddr_i, &lsu, 1 /* num_of_lsu */);

    // "Send();"
    Send(node, star);
}

//
// FUNCTION     NeighborUp()        - Figure 1. STAR Specification
// PURPOSE      Initialize STAR state variables
// PARAMETERS   star            - pointer for dataspace
//

static void
NeighborUp(
    Node *node,
    StarData *star,
    NodeAddress k,
    int       link_cost)
{
    BOOL sendST;
    StarLSU lsu;

    // "Neighbor protocol reports connectivity to neighbor k"

#ifdef DEBUG
    printf("Node Id %d #%d: Neighbor protocol reports"
           " connectivity to neighbor k (%d)\n",
            node->nodeId, star->ipAddr_i, k);
#endif

    // "Ni   <-- Ni U (k);"
    AddNeighborToNeighborSet(star, k, link_cost, STAR_CURRENT_CLOCK(), node);
    // "TGi(k) <-- 0;"
    AddTopologyGraph(star, k, node);
    // "STi  <-- 0;"
    AddShortestPathTree(star, k, node);

    // "STi(k) <-- 0;"

    // "sendST <-- TRUE;"
    sendST = TRUE;

    // "if (LORA and k in TGi and TGi(k).pred != null)"
    if (star->LORAmode &&
        ReturnTopologyGraphVertexEntry(&star->topologyGraph_i, k))
    {
        // "NSi   <-- TRUE;"
        star->NSi = TRUE;
        // "sendST <-- FALSE;"
        sendST = FALSE;
    }

    // "Update(i, (i,k,l(i)k,Ti));"
    lsu.ipAddr_u = star->ipAddr_i;
    lsu.ipAddr_v = k;
    lsu.link_cost_l = link_cost;
    lsu.timestamp_t = STAR_CURRENT_CLOCK();
    Update(node, star, star->ipAddr_i, &lsu, 1 /* num_of_lsu */);

    // "if (sendST)"
    if (sendST)
    {
        // "MSGi <-- 0;"
#ifdef DEBUG
        printf("\tMSGi <-- 0;\n");
#endif
        star->MSGi_numEntries = 0;


        // "for each (link (u,v) in STi)"
        // "    MSGi <-- MSGi U {(u,v,TGi(u,v).l, TGi(u,v).t)};"
        AddShortestPathTreeLinksToMSG(star, &(star->topoGraphST), node);

    }

    // "Send();"
    Send(node, star);
}


void StarNeighborUpdate(
    Node *node,
    NodeAddress neighbor,
    BOOL neighborIsReachable)
{
    StarData* star = (StarData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_STAR);

#ifdef DEBUG
    printf("#%d: StarNeighborUpdate(neighbor = %d) %s\n",
           node->nodeId, neighbor,
           (neighborIsReachable ? "TRUE" : "FALSE"));
#endif

    if (neighborIsReachable)
    {
        // Neighbor Up
        NeighborUp(node, star, neighbor, DEFAULT_LINK_COST);
    }
    else
    {
        // Neighbor Down
        NeighborDown(node, star, neighbor);
    }
}

//
// FUNCTION     StarInit()
// PURPOSE      Initialize STAR Routing Protocol Dataspace
// PARAMETERS   myRoute            - pointer for dataspace
//              nodeInput
//

void StarInit(
   Node *node,
   StarData** star,
   const NodeInput *nodeInput,
   int interfaceIndex)
{
    BOOL retVal;
    clocktype sendFrequency,
              entryTTL;
    char buf[MAX_STRING_LENGTH];
    NodeAddress interfaceAddress = 
                NetworkIpGetInterfaceAddress(node, interfaceIndex);
    if (MAC_IsWiredNetwork(node, interfaceIndex))
    {
        ERROR_Assert(FALSE, "STAR can only support wireless interfaces");
    }

    *star = (StarData *) MEM_malloc(sizeof(StarData));

    RANDOM_SetSeed((*star)->seed,
                   node->globalSeed,
                   node->nodeId,
                   ROUTING_PROTOCOL_STAR,
                   interfaceIndex);

    IO_ReadString(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "STAR-ROUTING-MODE",
        &retVal,
        buf);

    if (retVal == FALSE)
    {
        ERROR_ReportError("\"STAR-ROUTING-MODE\" needs to be either \"LORA\""
                          " or \"ORA\"");
    }

    if (strcmp(buf, "LORA") == 0)
    {
        (*star)->LORAmode = TRUE;
    }
    else if (strcmp(buf, "ORA") == 0)
    {
        (*star)->LORAmode = FALSE;
    }
    else
    {
        ERROR_ReportError("\"STAR-ROUTING-MODE\" needs to be either \"LORA\""
                          " or \"ORA\"");
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if ((retVal == FALSE) || (strcmp(buf, "NO") == 0))
    {
        (*star)->statsCollected = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        (*star)->statsCollected = TRUE;
        StarInitStats(&((*star)->stats));
    }
    else
    {
        ERROR_ReportError("\"ROUTING-STATISTICS\" needs to be either \"YES\""
                          " or \"NO\"");
    }


    /* Initialize IP forwarding table. */
    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_STAR);

    // We just use the IP forwarding table
    // NetworkIpSetRouterFunction(node, &StarRouterFunction, interfaceIndex);

    NetworkIpSetMacLayerStatusEventHandlerFunction(node,
                                                   &StarMacLayerStatusHandler,
                                                   interfaceIndex);

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "NEIGHBOR-PROTOCOL-SEND-FREQUENCY",
        &retVal,
        &sendFrequency);

    if (retVal == FALSE)
    {
        ERROR_ReportError("\"NEIGHBOR-PROTOCOL-SEND-FREQUENCY\" needs to be "
                          "specified.");
    }

    IO_ReadTime(
        node->nodeId,
        interfaceAddress,
        nodeInput,
        "NEIGHBOR-PROTOCOL-ENTRY-TTL",
        &retVal,
        &entryTTL);

    if (retVal == FALSE)
    {
        ERROR_ReportError("\"NEIGHBOR-PROTOCOL-ENTRY-TTL\" needs to be "
                          "specified.");
    }

    NeighborProtocolRegisterNeighborUpdateFunction(
        node,
        &StarNeighborUpdate,
        NetworkIpGetInterfaceAddress(node, interfaceIndex),
        sendFrequency,
        entryTTL);

#ifdef DEBUG
    printf("#%d: StarInit()\n", node->nodeId);
#endif

    NodeUp(*star, NetworkIpGetInterfaceAddress(node, interfaceIndex), node);
}


//
// FUNCTION     StarFinalize(Node* node)
// PURPOSE      Finalize statistics Collection
//

void StarFinalize(Node *node)
{
    StarData* star = (StarData *)
                      NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_STAR);
    char buf[MAX_STRING_LENGTH];

#ifdef BROKEN_DEBUG
    printf("#%d: StarFinalize(Node* node)\n", node->nodeId);

    printf("The topology graph(ST) is:\n");
    PrintTopologyGraph(star->ipAddr_i, &star->topoGraphST);

    printf("The topology graph(i) is:\n");
    PrintTopologyGraph(star->ipAddr_i, &star->topologyGraph_i);

    printf("The topology graph(ST-prime) is:\n");
    PrintTopologyGraph(star->ipAddr_i, &star->topoGraphST_prime);

    printf("The total (Topograph-K) is: %d\n",star->numTopoGraph_k);
        for(int i=0; i < star->numTopoGraph_k; i++)
        {
            printf("The topology graph(Topograph-K) is:\n");
            PrintTopologyGraph(star->ipAddr_i, star->topoGraph_k);
            star->topoGraph_k++;
        }

    printf("The total (Shortest-Path-K) is: %d\n",star->numST_k);
        for(i=0; i < star->numST_k; i++)
        {
            printf("The topology graph(ShortestPathTree-K) is:\n");
            PrintTopologyGraph(star->ipAddr_i, star->ShortestPathTree_k);
            star->ShortestPathTree_k++;
        }
#endif

    EmptyTopologyGraph(&star->topoGraphST, node);
    EmptyTopologyGraph(&star->topologyGraph_i, node);
    EmptyTopologyGraph(&star->topoGraphST_prime, node);

    StarTopologyGraph *tempGraph = star->ShortestPathTree_k;
    int i = 0;
    for(i = 0; i < star->numST_k; i++)
    {
        EmptyTopologyGraph(star->ShortestPathTree_k, node);
        star->ShortestPathTree_k++;
    }
    MEM_free(tempGraph);

    tempGraph = star->topoGraph_k;
    for(i = 0; i < star->numTopoGraph_k; i++)
    {
        EmptyTopologyGraph(star->topoGraph_k, node);
        star->topoGraph_k++;
    }
    MEM_free(tempGraph);

    if (star->statsCollected)
    {
        sprintf(buf, "Update Packets Sent = %d",
                star->stats.numPkts_sent);
        IO_PrintStat(
            node,
            "Network",
            "STAR",
            ANY_DEST,
            -1 /* instance Id */,
            buf);

        sprintf(buf, "Update Packets Received = %d",
                star->stats.numPkts_recvd);
        IO_PrintStat(
            node,
            "Network",
            "STAR",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
        sprintf(buf, "Link State Updates Sent = %d",
                star->stats.numLSU_sent);
        IO_PrintStat(
            node,
            "Network",
            "STAR",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
        sprintf(buf, "Link State Updates Received = %d",
                star->stats.numLSU_recvd);
        IO_PrintStat(
            node,
            "Network",
            "STAR",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
    }
}


//
// FUNCTION     StarHandleProtocolPacket(Node* node, Message* msg,
//                                      NodeAddress sourceAddress)
// PURPOSE      Process a STAR generated control packet
// PARAMETERS   msg             - The packet
//

void StarHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress)
{
    StarData* star = (StarData *)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_STAR);

    StarTopologyGraph *topologyGraph_srcAddr;

#ifdef DEBUG
    printf("#%d:\tSTAR packet.\n",node->nodeId);
#endif

    topologyGraph_srcAddr = ReturnTopologyGraph(
                                star,
                                sourceAddress);

    if (!topologyGraph_srcAddr)
    {
        // Neighbor Protocol is too slow: here is a new neighbor
        NeighborUp(node, star, sourceAddress, DEFAULT_LINK_COST);
    }

    star->stats.numPkts_recvd++;
    star->stats.numLSU_recvd += (MESSAGE_ReturnPacketSize(msg)
                                  / sizeof(StarLSU));
    Update(
        node,
        star,
        sourceAddress,
        (StarLSU *) MESSAGE_ReturnPacket(msg),
        (MESSAGE_ReturnPacketSize(msg) / sizeof(StarLSU)) /* num_of_lsu */);
    MESSAGE_Free(node, msg);
}

//
// FUNCTION     StarHandleProtocolEvent(Node* node, Message* msg)
// PURPOSE      Process timeouts sent by STAR to itself
// PARAMETERS   msg             - the timer
//

void StarHandleProtocolEvent(Node* node, Message* msg) {

    // NodeAddress *info = (NodeAddress *)MESSAGE_ReturnInfo(msg);
    // StarData *star;

    // star = (StarData *)
    //        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_STAR);

    MESSAGE_Free(node, msg);
}



//
// FUNCTION     StarRouterFunction()
// PURPOSE      Determine the routing action to take for a the given data
//              packet, and set the PacketWasRouted variable to TRUE if no
//              further handling of this packet by IP is necessary.
// PARAMETERS   msg             - the data packet
//              destAddr        - the destination node of this data packet
//              previousHopAddr - previous hop of packet
//              PacketWasRouted - variable that indicates to IP that it
//                                no longer needs to act on this data packet
//

void StarRouterFunction(Node *node,
                        Message *msg,
                        NodeAddress destAddr,
                        NodeAddress previousHopAddr,
                        BOOL* PacketWasRouted)
{
    // StarData *star;
    // IpHeaderType *ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    // star = (StarData *)
    //        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_STAR);

    *PacketWasRouted = FALSE;
    return;
}


//
// FUNCTION     StarMacLayerStatusHandler()
// PURPOSE      Handle internal messages from MAC Layer
// PARAMETERS   msg             - the packet returned by MAC
//

void StarMacLayerStatusHandler(Node *node,
                               const Message* msg,
                               const NodeAddress nextHopAddress,
                               const int interfaceIndex)
{

    switch(MESSAGE_GetEvent(msg))
    {
        case MSG_NETWORK_PacketDropped:
        {
#ifdef DEBUG
            printf("    PacketDropped.\n");
#endif
            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "#%d: Cannot handle message type #%d\n",
                    node->nodeId, MESSAGE_GetEvent(msg));

            ERROR_ReportError(errorStr);
        }
    }
}
