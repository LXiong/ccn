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


#ifndef __UTIL_TLV_T__
# define __UTIL_TLV_T__

# ifdef __UTIL_TLV_C__
UTIL_Tlv UTIL_TlvNew(int type, void *data) ;
void UTIL_TlvInit(void) ;
void UTIL_TlvReady(void) ;
void UTIL_TlvEpoch(void) ;
void *UTIL_TlvRegisterVariable(char *varName, void **defValue) ;
UTIL_TlvDispatchEntry UTIL_TlvDispatchEntryNew(UTIL_TlvSerializer ser, UTIL_TlvDeserializer des, UTIL_TlvOperator oper) ;
UTIL_String UTIL_TlvTupletFormat(char *attr, char *val);
void UTIL_TlvRegisterDefaultTlvTypes() ;
void UTIL_TlvRegisterSerDes(int type, UTIL_TlvSerializer ser, UTIL_TlvDeserializer des, UTIL_TlvOperator oper) ;
void UTIL_TlvDeregisterSerDes(int type) ;
UTIL_TlvSerializer UTIL_TlvGetSerializer(int type) ;
UTIL_TlvDeserializer UTIL_TlvGetDeserializer(int type) ;
UTIL_TlvOperator UTIL_TlvGetOperator(int type) ;
UTIL_Tlv UTIL_TlvOperate(UTIL_Tlv tlv, PartitionData *partitionData) ;
void UTIL_TlvPush(UTIL_Tlv tlv, UTIL_Buffer buf) ;
UTIL_Tlv UTIL_TlvPop(UTIL_Buffer buf) ;
static void UTIL_TlvFinalize(void *ptr) ;
static void UTIL_TlvDispatchEntryFinalize(void *ptr) ;
static char *UTIL_TlvNullSerializer(void *obj, int *size) ;
static void *UTIL_TlvNullDeserializer(char *buf) ;
static UTIL_Tlv UTIL_TlvNullOperator(UTIL_Tlv tlv, PartitionData *partitionData);
static char *UTIL_TlvStringSerializer(void *obj, int *size) ;
static void *UTIL_TlvStringDeserializer(char *buf) ;
static UTIL_Tlv UTIL_TlvStringOperator(UTIL_Tlv tlv, PartitionData *partitionData);
static char *UTIL_TlvTupletSerializer(void *obj, int *size) ;
static void *UTIL_TlvTupletDeserializer(char *buf) ;
static void UTIL_TlvTupletParse(char *str, char* &attr, char* &val);
static UTIL_Tlv UTIL_TlvTupletOperator(UTIL_Tlv tlv, PartitionData *partitionData);
static char *UTIL_TlvGetStringSerializer(void *obj, int *size) ;
static void *UTIL_TlvGetStringDeserializer(char *buf) ;
static UTIL_Tlv UTIL_TlvGetStringOperator(UTIL_Tlv tlv, PartitionData *partitionData);
static char *UTIL_TlvSetTupletSerializer(void *obj, int *size);
static void *UTIL_TlvSetTupletDeserializer(char *buf);
static UTIL_Tlv UTIL_TlvSetTupletOperator(UTIL_Tlv tlv, PartitionData *partitionData);
static char *UTIL_TlvGetResponseTupletSerializer(void *obj, int *size);
static void *UTIL_TlvGetResponseTupletDeserializer(char *buf);
static UTIL_Tlv UTIL_TlvGetResponseTupletOperator(UTIL_Tlv tlv, PartitionData *partitionData);
static char *UTIL_TlvCmdResponseTupletSerializer(void *obj, int *size);
static void *UTIL_TlvCmdResponseTupletDeserializer(char *buf) ;
static UTIL_Tlv UTIL_TlvCmdResponseTupletOperator(UTIL_Tlv tlv, PartitionData *partitionData) ;
static char *UTIL_TlvCmdStringSerializer(void *obj, int *size) ;
static void *UTIL_TlvCmdStringDeserializer(char *buf) ;
static UTIL_Tlv UTIL_TlvCmdStringOperator(UTIL_Tlv tlv, PartitionData *partitionData);
# else /* __UTIL_TLV_C__ */
extern UTIL_Tlv UTIL_TlvNew(int type, void *data) ;
extern void UTIL_TlvInit(void) ;
extern void UTIL_TlvReady(void) ;
extern void UTIL_TlvEpoch(void) ;
extern void *UTIL_TlvRegisterVariable(char *varName, void **defValue) ;
extern UTIL_TlvDispatchEntry UTIL_TlvDispatchEntryNew(UTIL_TlvSerializer ser, UTIL_TlvDeserializer des, UTIL_TlvOperator oper) ;
extern UTIL_String UTIL_TlvTupletFormat(char *attr, char *val);
extern void UTIL_TlvRegisterDefaultTlvTypes() ;
extern void UTIL_TlvRegisterSerDes(int type, UTIL_TlvSerializer ser, UTIL_TlvDeserializer des, UTIL_TlvOperator oper) ;
extern void UTIL_TlvDeregisterSerDes(int type) ;
extern UTIL_TlvSerializer UTIL_TlvGetSerializer(int type) ;
extern UTIL_TlvDeserializer UTIL_TlvGetDeserializer(int type) ;
extern UTIL_TlvOperator UTIL_TlvGetOperator(int type) ;
extern UTIL_Tlv UTIL_TlvOperate(UTIL_Tlv tlv, PartitionData *partitionData) ;
extern void UTIL_TlvPush(UTIL_Tlv tlv, UTIL_Buffer buf) ;
extern UTIL_Tlv UTIL_TlvPop(UTIL_Buffer buf) ;
# endif /* __UTIL_TLV_C__ */
#endif /* __UTIL_TLV_T__ */
