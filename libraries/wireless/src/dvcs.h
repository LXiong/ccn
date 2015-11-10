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

#ifndef DVCS_H
#define DVCS_H

//------------------------------------------------------------------------------

typedef struct DirectionalNavCellStruct {
    double leftSectorAngle;
    double rightSectorAngle;
    clocktype expirationTime;
    struct DirectionalNavCellStruct* next;
} DirectionalNavCellType;

typedef struct {
    DirectionalNavCellType* listHeadPtr;
    DirectionalNavCellType* freeListPtr;
} DirectionalNavType;

static //inline//
void DirectionalNav_Init(DirectionalNavType* directionalNav) {
   directionalNav->listHeadPtr = NULL;
   directionalNav->freeListPtr = NULL;
}

static //inline//
void DirectionalNav_Add(
   DirectionalNavType* directionalNav,
   const double leftSectorAngle,
   const double rightSectorAngle,
   const clocktype expirationTime,
   const clocktype currentTime)
{
   DirectionalNavCellType* newCell = NULL;

   if (directionalNav->freeListPtr == NULL) {
      newCell = (DirectionalNavCellType*)
         MEM_malloc(sizeof(DirectionalNavCellType));
   } else {
      newCell = directionalNav->freeListPtr;
      directionalNav->freeListPtr = directionalNav->freeListPtr->next;
   }//if//

   newCell->leftSectorAngle = leftSectorAngle;
   newCell->rightSectorAngle = rightSectorAngle;
   newCell->expirationTime = expirationTime;
   newCell->next = directionalNav->listHeadPtr;
   directionalNav->listHeadPtr = newCell;
}

static //inline//
double NormalizedAngle(double angle) {
   if (angle < 0.0) {
      angle = angle + 360.0;
   } else if (angle >= 360.0) {
      angle = angle - 360.0;
   }//if//

   assert((angle >= 0.0) && (angle < 360.0));
   return angle;
}


static //inline//
BOOL AngleIsInSector(
    const double leftSectorAngle,
    const double rightSectorAngle,
    const double theAngle)
{
   assert((leftSectorAngle >= 0.0) && (leftSectorAngle < 360.0));
   assert((rightSectorAngle >= 0.0) && (rightSectorAngle < 360.0));
   assert((theAngle >= 0.0) && (theAngle < 360.0));


   if (leftSectorAngle < rightSectorAngle) {
      return ((theAngle >= leftSectorAngle) && (theAngle <= rightSectorAngle));
   } else {
      return !((theAngle >= rightSectorAngle) &&
               (theAngle <= leftSectorAngle));
   }//if//
}


static //inline//
BOOL DirectionalNav_SignalIsNaved(
   const DirectionalNavType* directionalNav,
   const double signalAngleOfArrival,
   const clocktype currentTime)
{
   DirectionalNavCellType* current = directionalNav->listHeadPtr;
   while (current != NULL) {
      if ((current->expirationTime > currentTime) &&
          (AngleIsInSector(current->leftSectorAngle,
                           current->rightSectorAngle,
                           signalAngleOfArrival)))
      {
         //printf("Signal was Directionally NAVed!!!\n");
         return TRUE;
      }//if//
      current = current->next;
   }//while//
   return FALSE;
}


static //inline//
void DirectionalNav_GetEarliestNavExpirationTime(
   DirectionalNavType* directionalNav,
   const clocktype currentTime,
   clocktype* earliestNavTime)
{
   clocktype earliestTime = CLOCKTYPE_MAX;
   DirectionalNavCellType* current = directionalNav->listHeadPtr;
   DirectionalNavCellType* prevCell = NULL;
   while (current != NULL) {
      if (current->expirationTime <= currentTime) {
         if (prevCell == NULL) {
            directionalNav->listHeadPtr = current->next;
            current->next = directionalNav->freeListPtr;
            directionalNav->freeListPtr = current;
            current = directionalNav->listHeadPtr;
         } else {
            prevCell->next = current->next;
            current->next = directionalNav->freeListPtr;
            directionalNav->freeListPtr = current;
            current = prevCell->next;
         }//if//
      } else {
         if (current->expirationTime < earliestTime) {
            earliestTime = current->expirationTime;
         }//if//
         prevCell = current;
         current = current->next;
      }//if//
   }//while//

   *earliestNavTime = earliestTime;
}



static //inline//
BOOL DirectionalNav_ThereAreNoNavs(
   const DirectionalNavType* directionalNav,
   const clocktype currentTime)
{
   DirectionalNavCellType* current = directionalNav->listHeadPtr;
   while (current != NULL) {
      if (current->expirationTime > currentTime) {
         return FALSE;
      }//if//
      current = current->next;
   }//while//
   return TRUE;
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

typedef struct AngleOfArrivalCacheCellStruct {
    Mac802Address destinationMacId;
    double arrivalAngle;
    clocktype expirationTime;
    struct AngleOfArrivalCacheCellStruct* next;
} AngleOfArrivalCacheCellType;

typedef struct {
    // after 100 updates prune the cache
    int updates;
    std::map<Mac802Address, AngleOfArrivalCacheCellType>* cache;
} AngleOfArrivalCacheType;

typedef struct {
    AngleOfArrivalCacheType angleOfArrivalCache;
    clocktype defaultCacheExpirationDelay;
    DirectionalNavType directionalNav;
    double defaultNavedDeltaAngle;
} DirectionalAntennaMacInfoType;


static //inline//
void AngleOfArrivalCache_Init(AngleOfArrivalCacheType* cache) {
   cache->updates = 0;
   cache->cache = new std::map<Mac802Address, AngleOfArrivalCacheCellType>;
}

static //inline//
void AngleOfArrivalCache_Add(
   AngleOfArrivalCacheType* angleOfArrivalCache,
   const Mac802Address destinationMacId,
   const double arrivalAngle,
   const clocktype expirationTime)
{
   // Assume there is no entry for this destination.

   AngleOfArrivalCacheCellType cellToUpdate;

   cellToUpdate.destinationMacId = destinationMacId;
   cellToUpdate.arrivalAngle = arrivalAngle;
   cellToUpdate.expirationTime = expirationTime;

   (*angleOfArrivalCache->cache)[destinationMacId] = cellToUpdate;
}

static //inline//
void AngleOfArrivalCache_Add(
   AngleOfArrivalCacheType* angleOfArrivalCache,
   const NodeAddress destinationMacId,
   const double arrivalAngle,
   const clocktype expirationTime)
{
    Mac802Address destination802MacId;
    memset(destination802MacId.byte, 0, MAC_ADDRESS_LENGTH_IN_BYTE);
    memcpy(destination802MacId.byte,&destinationMacId,sizeof(NodeAddress));

    AngleOfArrivalCache_Add(
        angleOfArrivalCache,
        destination802MacId,
        arrivalAngle,
        expirationTime);
}

static //inline//
void AngleOfArrivalCache_Update(
   AngleOfArrivalCacheType* angleOfArrivalCache,
   const Mac802Address destinationMacId,
   const double arrivalAngle,
   BOOL* nodeIsInCache,
   const clocktype expirationTime,
   const clocktype currentTime)
{
   // Remove Expired Entries.

   // Update the cache after 100 updates.  Oherwise just add.
   // There could be a better way to do this but it's ok for now.
   angleOfArrivalCache->updates++;
   if (angleOfArrivalCache->updates < 100)
   {
       AngleOfArrivalCache_Add(
           angleOfArrivalCache,
           destinationMacId,
           arrivalAngle,
           expirationTime);
       return;
      }
   angleOfArrivalCache->updates = 0;

   std::map<Mac802Address, AngleOfArrivalCacheCellType>::iterator it;
   std::map<Mac802Address, AngleOfArrivalCacheCellType>::iterator it2;
   AngleOfArrivalCacheCellType* cell;

   // Assume not in cache until found
       *nodeIsInCache = FALSE;

   it = angleOfArrivalCache->cache->begin();
   while (it != angleOfArrivalCache->cache->end())
   {
       cell = &it->second;

       if (cell->destinationMacId == destinationMacId)
       {
           *nodeIsInCache = TRUE;
           cell->destinationMacId = destinationMacId;
           cell->arrivalAngle = arrivalAngle;
           cell->expirationTime = expirationTime;

           it++;
       }
       else if (currentTime >= cell->expirationTime)
       {
           // Remove
           it2 = it;
           it++;
           angleOfArrivalCache->cache->erase(it2);
       }
       else
       {
           it++;
       }
   }

   if (!*nodeIsInCache)
   {
       AngleOfArrivalCache_Add(
           angleOfArrivalCache,
           destinationMacId,
           arrivalAngle,
           expirationTime);
   }
}

static //inline//
void AngleOfArrivalCache_Update(
   AngleOfArrivalCacheType* angleOfArrivalCache,
   const NodeAddress destinationMacId,
   const double arrivalAngle,
   BOOL* nodeIsInCache,
   const clocktype expirationTime,
   const clocktype currentTime)
{
    Mac802Address destination802MacId;
    memset(destination802MacId.byte, 0, MAC_ADDRESS_LENGTH_IN_BYTE);
    memcpy(destination802MacId.byte,&destinationMacId,sizeof(NodeAddress));

    AngleOfArrivalCache_Update(
       angleOfArrivalCache,
       destination802MacId,
       arrivalAngle,
       nodeIsInCache,
       expirationTime,
       currentTime);
}

static //inline//
void AngleOfArrivalCache_GetAngle(
   const AngleOfArrivalCacheType* AngleOfArrivalCache,
   const Mac802Address destinationMacId,
   const clocktype currentTime,
   BOOL* nodeIsInCache,
   double* arrivalAngle)
{
   std::map<Mac802Address, AngleOfArrivalCacheCellType>::iterator it;
       
   it = AngleOfArrivalCache->cache->find(destinationMacId);
   if (it == AngleOfArrivalCache->cache->end())
   {
   *nodeIsInCache = FALSE;
}
   else
   {
       *arrivalAngle = it->second.arrivalAngle;
       *nodeIsInCache = currentTime < it->second.expirationTime;
   }
}

static //inline//
void AngleOfArrivalCache_GetAngle(
   const AngleOfArrivalCacheType* AngleOfArrivalCache,
   const NodeAddress destinationMacId,
   const clocktype currentTime,
   BOOL* nodeIsInCache,
   double* arrivalAngle)
{
    Mac802Address destination802MacId;
    memset(destination802MacId.byte, 0, MAC_ADDRESS_LENGTH_IN_BYTE);
    memcpy(destination802MacId.byte,&destinationMacId,sizeof(NodeAddress));

   AngleOfArrivalCache_GetAngle(
        AngleOfArrivalCache,
        destination802MacId,
        currentTime,
        nodeIsInCache,
        arrivalAngle);
}

static //inline//
void AngleOfArrivalCache_Delete(
   AngleOfArrivalCacheType* AngleOfArrivalCache,
   const Mac802Address destinationMacId)
{
   std::map<Mac802Address, AngleOfArrivalCacheCellType>::iterator it;

   it = AngleOfArrivalCache->cache->find(destinationMacId);
   if (it != AngleOfArrivalCache->cache->end())
   {
       AngleOfArrivalCache->cache->erase(it);
   }
}

static //inline//
void AngleOfArrivalCache_Delete(
   AngleOfArrivalCacheType* AngleOfArrivalCache,
   const NodeAddress destinationMacId)
{
    Mac802Address destination802MacId;
    memset(destination802MacId.byte, 0, MAC_ADDRESS_LENGTH_IN_BYTE);
    memcpy(destination802MacId.byte,&destinationMacId,sizeof(NodeAddress));

    AngleOfArrivalCache_Delete(
        AngleOfArrivalCache,
        destination802MacId);
}

//-----------------------------------------------------

static //inline//
void InitDirectionalAntennalMacInfo(
   DirectionalAntennaMacInfoType* directionalAntennalInfo,
   const clocktype defaultCacheExpirationDelay,
   const double defaultNavedDeltaAngle)
{
   DirectionalNav_Init(&directionalAntennalInfo->directionalNav);
   AngleOfArrivalCache_Init(&directionalAntennalInfo->angleOfArrivalCache);

   directionalAntennalInfo->defaultCacheExpirationDelay =
      defaultCacheExpirationDelay;
   directionalAntennalInfo->defaultNavedDeltaAngle = defaultNavedDeltaAngle;

}

#endif /*DVCS_H*/
