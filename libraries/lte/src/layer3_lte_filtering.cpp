#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"

#include "layer2_lte.h"
#include "lte_rrc_config.h"
#include "layer3_lte_filtering.h"

LteMeasurementValue::LteMeasurementValue()
{
}

LteMeasurementValue::~LteMeasurementValue()
{
}

// /**
// FUNCTION   :: LteMeasurementValue::update
// PURPOSE    ::
// + insert new value and calculate {exponential/arithmetic} mean
// + used at both case of update or not update filterCoefficient
// PARAMETERS ::
// + [in] double : : value
// + [in] double : : filterCoef
// + [in] bool : : updateFilterCoef
// RETURN     :: BOOL : TURE(update success) / FALSE(update failure)
// **/
void LteMeasurementValue::update(
                    double value, double filterCoef, bool updateFilterCoef)
{
    _exponentialMean.update(value, filterCoef, updateFilterCoef);
    _arithmeticMean.update(value);
}

// /**
// FUNCTION   :: LteMeasurementValue::get
// PURPOSE    ::
// + get {exponential/arithmetic} mean value
// PARAMETERS ::
// + [out] double* : : value
// + [in] LteMeanType : : meanType
// RETURN     :: BOOL : TURE(get success) / FALSE(value not found)
// **/
BOOL LteMeasurementValue::get(double* value, LteMeanType meanType)
{
    BOOL retval = TRUE;

    switch (meanType)
    {
    case EXPONENTIAL_MEAN:
        _exponentialMean.get(value);
        retval = TRUE;
        break;

    case ARITHMETIC_MEAN:
        _arithmeticMean.get(value);
        retval = TRUE;
        break;

    default:
        retval = FALSE;
//        *value = L3_FILTER_NOT_FOUND_VALUE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteMeasurementValue::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (all entries)
// PARAMETERS :: None
// RETURN     :: None
// **/
void LteMeasurementValue::reset()
{
    // reset all meanType
    for (int meanType = 0; meanType < MEAN_TYPE_NUM; meanType++)
    {
        reset((LteMeanType)meanType);
    }
}

// /**
// FUNCTION   :: LteMeasurementValue::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (specific MeasurementType, MeanType)
// PARAMETERS ::
// + LteMeanType : : meanType
// RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
// **/
BOOL LteMeasurementValue::reset(LteMeanType meanType)
{
    BOOL retval = TRUE;

    switch (meanType)
    {
    case EXPONENTIAL_MEAN:
        _exponentialMean.reset();
        retval = TRUE;
        break;

    case ARITHMETIC_MEAN:
        _arithmeticMean.reset();
        retval = TRUE;
        break;

    default:
        retval = FALSE;
    }

    return retval;
}

LteMeasurementData::LteMeasurementData(const Layer2DataLte* layer2DateLte)
{
    _layer2DateLte = layer2DateLte;
    _mapMeasValue.clear();
}

LteMeasurementData::~LteMeasurementData()
{
    for (MapLteMeasurementValue::iterator itr = _mapMeasValue.begin();
         itr != _mapMeasValue.end();
         itr++)
    {
        delete itr->second; // delete LteMeasurementValue
    }
    _mapMeasValue.clear();
}

// /**
// FUNCTION   :: LteMeasurementData::update
// PURPOSE    ::
// + insert new value and calculate {exponential/arithmetic} mean
// + used at both case of update or not update filterCoefficient
// PARAMETERS ::
// + [in] LteMeasurementType : : measType
// + [in] double : : value
// + [in] double : : filterCoef
// + [in] bool : : updateFilterCoef
// RETURN     :: BOOL : TURE(update success) / FALSE(update failure)
// **/
BOOL LteMeasurementData::update(
    LteMeasurementType measType,
    double value,
    double filterCoef,
    bool updateFilterCoef)
{
    BOOL retval = TRUE;

    // find measValue
    MapLteMeasurementValue::iterator itr = _mapMeasValue.find(measType);

    if (itr != _mapMeasValue.end())
    {
        // update measurement value
        itr->second->update(value, filterCoef, updateFilterCoef);
        retval = TRUE;
    }
    else
    {
        // create new measurement value
        LteMeasurementValue* newMeasValue =
            new LteMeasurementValue();

        if (updateFilterCoef)
        {
            // update measurement value(and FilterCoef)
            newMeasValue->update(value, filterCoef, updateFilterCoef);
        }
        else
        {
            // get FilterCoef
            filterCoef = _layer2DateLte->rrcConfig->filterCoefficient;

            // update measurement value(and FilterCoef)
            newMeasValue->update(value, filterCoef, true);
        }

        // insert
        if (_mapMeasValue.insert(
                MapLteMeasurementValue::value_type(measType, newMeasValue)
                ).second == false)
        {
            ERROR_ReportError("insert failed.");
        }
        else
        {
            retval = TRUE;
        }
    }

    return retval;
}

// /**
// FUNCTION   :: LteMeasurementData::get
// PURPOSE    ::
// + get {exponential/arithmetic} mean value
// PARAMETERS ::
// + [in] LteMeasurementType : : measType
// + [out] double* : : value
// + [in] LteMeanType : : meanType
// RETURN     :: BOOL : TURE(get success) / FALSE(value not found)
// **/
BOOL LteMeasurementData::get(
    LteMeasurementType measType,
    double* value,
    LteMeanType meanType)
{
    BOOL retval = TRUE;

    // find specified measValue
    MapLteMeasurementValue::iterator itr = _mapMeasValue.find(measType);

    if (itr != _mapMeasValue.end())
    {
        // find specified meanType
        retval = itr->second->get(value, meanType);
    }
    else
    {
        retval = FALSE;
//        *value = L3_FILTER_NOT_FOUND_VALUE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteMeasurementData::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (all entries)
// PARAMETERS :: None
// RETURN     :: None
// **/
void LteMeasurementData::reset()
{
    // reset all measValue
    for (MapLteMeasurementValue::iterator itr = _mapMeasValue.begin();
          itr != _mapMeasValue.end();
          itr++)
    {
        // reset all meanType
        itr->second->reset();
    }

}

// /**
// FUNCTION   :: LteMeasurementData::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (specific MeasurementType)
// PARAMETERS ::
// + LteMeasurementType : : measType
// RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
// **/
BOOL LteMeasurementData::reset(LteMeasurementType measType)
{
    BOOL retval = TRUE;

    // find specified maesValue
    MapLteMeasurementValue::iterator itr = _mapMeasValue.find(measType);

    if (itr != _mapMeasValue.end())
    {
        // reset all meanType
        itr->second->reset();
        retval = TRUE;
    }
    else
    {
        // not found
        retval = FALSE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteMeasurementData::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (specific MeasurementType, MeanType)
// PARAMETERS ::
// + LteMeasurementType : : measType
// + LteMeanType : : meanType
// RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
// **/
BOOL LteMeasurementData::reset(
    LteMeasurementType measType,
    LteMeanType meanType)
{
    BOOL retval = TRUE;

    // find specified measValue
    MapLteMeasurementValue::iterator itr = _mapMeasValue.find(measType);

    if (itr != _mapMeasValue.end())
    {
        // reset specified meanType
        retval = itr->second->reset(meanType);
    }
    else
    {
        // not found
        retval = FALSE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteMeasurementData::remove
// PURPOSE    ::
// + remove {exponential/arithmetic} mean (all entries)
// PARAMETERS :: None
// RETURN     :: None
// **/
void LteMeasurementData::remove()
{
    // remove all measValue
    for (MapLteMeasurementValue::iterator itr = _mapMeasValue.begin();
        itr != _mapMeasValue.end();
        itr++)
    {
        delete itr->second; // delete LteMeasurementValue
    }
    _mapMeasValue.clear();
}

// /**
// FUNCTION   :: LteMeasurementData::remove
// PURPOSE    ::
// + remove {exponential/arithmetic} mean (specific MeasurementType)
// PARAMETERS ::
// + LteMeasurementType : : measType
// RETURN     :: BOOL : TURE(remove success) / FALSE(target not found)
// **/
BOOL LteMeasurementData::remove(LteMeasurementType measType)
{
    BOOL retval = TRUE;

    // find specified measValue
    MapLteMeasurementValue::iterator itr = _mapMeasValue.find(measType);

    if (itr != _mapMeasValue.end())
    {
        delete itr->second; // delete LteMeasurementValue
        _mapMeasValue.erase(itr);
        retval = TRUE;
    }
    else
    {
        // not found
        retval = FALSE;
    }

    return retval;
}

LteLayer3Filtering::LteLayer3Filtering(const Layer2DataLte* layer2DateLte)
{
    _mapMeasData.clear();
    _layer2DateLte = layer2DateLte;
}

LteLayer3Filtering::~LteLayer3Filtering()
{
    for (MapLteMeasurementData::iterator itr = _mapMeasData.begin();
         itr != _mapMeasData.end();
         itr++)
    {
        delete itr->second; // delete LteMeasurementData
    }
    _mapMeasData.clear();
}

// /**
// FUNCTION   :: LteLayer3Filtering::update
// PURPOSE    ::
// + insert new value and calculate {exponential/arithmetic} mean
// + used at not update filterCoefficient
// PARAMETERS ::
// + [in] LteRnti : : oppositeRnti
// + [in] LteMeasurementType : : measType
// + [in] double : : value
// RETURN     :: BOOL : TURE(update success) / FALSE(update failure)
// **/
BOOL LteLayer3Filtering::update(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType,
    const double value)
{
    return updateExec(oppositeRnti, measType, value, 0.0, false);
}

// /**
// FUNCTION   :: LteLayer3Filtering::update
// PURPOSE    ::
// + insert new value and calculate {exponential/arithmetic} mean
// + used at update filterCoefficient
// PARAMETERS ::
// + [in] LteRnti : : oppositeRnti
// + [in] LteMeasurementType : : measType
// + [in] double : : value
// + [in] double : : filterCoef
// RETURN     :: BOOL : TURE(update success) / FALSE(update failure)
// **/
BOOL LteLayer3Filtering::update(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType,
    const double value,
    double filterCoef)
{
    return updateExec(oppositeRnti, measType, value, filterCoef, true);
}

// /**
// FUNCTION   :: LteLayer3Filtering::update
// PURPOSE    ::
// + insert new value and calculate {exponential/arithmetic} mean
// + used at both case of update or not update filterCoefficient
// PARAMETERS ::
// + [in] LteRnti : : oppositeRnti
// + [in] LteMeasurementType : : measType
// + [in] double : : value
// + [in] double : : filterCoef
// + [in] bool : : updateFilterCoef
// RETURN     :: BOOL : TURE(update success) / FALSE(update failure)
// **/
BOOL LteLayer3Filtering::updateExec(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType,
    double value,
    double filterCoef,
    bool updateFilterCoef)
{
    BOOL retval = TRUE;

    // find specified measData
    MapLteMeasurementData::iterator itr = _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        itr->second->update(measType, value, filterCoef, updateFilterCoef);
    }
    else
    {
        // create new measData
        LteMeasurementData* newMeasData =
                                    new LteMeasurementData(_layer2DateLte);

        newMeasData->update(measType, value, filterCoef, updateFilterCoef);
        // insert
        if (_mapMeasData.insert(
                MapLteMeasurementData::value_type(oppositeRnti, newMeasData)
                ).second == false)
        {
            ERROR_ReportError("insert failed.");
        }
        else
        {
            retval = TRUE;
        }
    }

    return retval;
}

// /**
// FUNCTION   :: LteLayer3Filtering::get
// PURPOSE    ::
// + get {exponential/arithmetic} mean value
// PARAMETERS ::
// + [in] LteRnti : : oppositeRnti
// + [in] LteMeasurementType : : measType
// + [out] double* : : value
// RETURN     :: BOOL : TURE(get success) / FALSE(value not found)
// **/
BOOL LteLayer3Filtering::get(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType,
    double* value) const
{
    return get(oppositeRnti, measType, value, EXPONENTIAL_MEAN);
}

// /**
// FUNCTION   :: LteLayer3Filtering::get
// PURPOSE    ::
// + get {exponential/arithmetic} mean value
// PARAMETERS ::
// + [in] LteRnti : : oppositeRnti
// + [in] LteMeasurementType : : measType
// + [out] double* : : value
// + [in] LteMeanType : : meanType
// RETURN     :: BOOL : TURE(get success) / FALSE(value not found)
// **/
BOOL LteLayer3Filtering::get(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType,
    double* value,
    const LteMeanType meanType) const
{
    BOOL retval = TRUE;

    // find specified oppositeRnti
    MapLteMeasurementData::const_iterator itr =
                                            _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        retval = itr->second->get(measType, value, meanType);
    }
    else
    {
        retval = FALSE;
//        *value = L3_FILTER_NOT_FOUND_VALUE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteLayer3Filtering::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (all entries)
// PARAMETERS :: None
// RETURN     :: None
// **/
void LteLayer3Filtering::reset()
{
    // reset all measData
    for (MapLteMeasurementData::iterator itr = _mapMeasData.begin();
        itr != _mapMeasData.end();
        itr++)
    {
        // reset all measValue
        itr->second->reset();
    }
}

// /**
// FUNCTION   :: LteLayer3Filtering::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (specific RNTI)
// PARAMETERS ::
// + LteRnti : : oppositeRnti
// RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
// **/
BOOL LteLayer3Filtering::reset(
    const LteRnti& oppositeRnti)
{
    BOOL retval = TRUE;

    // find specified measData
    MapLteMeasurementData::iterator itr = _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        // reset all measValue
        itr->second->reset();
        retval = TRUE;
    }
    else
    {
        retval = FALSE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteLayer3Filtering::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean (specific RNTI, MeasurementType)
// PARAMETERS ::
// + LteRnti : : oppositeRnti
// + LteMeasurementType : : measType
// RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
// **/
BOOL LteLayer3Filtering::reset(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType)
{
    BOOL retval = TRUE;

    // find specified measData
    MapLteMeasurementData::iterator itr = _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        retval = itr->second->reset(measType);
    }
    else
    {
        retval = FALSE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteLayer3Filtering::reset
// PURPOSE    ::
// + reset {exponential/arithmetic} mean
//         (specific RNTI, MeasurementType, MeanType)
// PARAMETERS ::
// + LteRnti : : oppositeRnti
// + LteMeasurementType : : measType
// + LteMeanType : : meanType
// RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
// **/
BOOL LteLayer3Filtering::reset(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType,
    const LteMeanType meanType)
{
    BOOL retval = TRUE;

    // find specified measData
    MapLteMeasurementData::iterator itr = _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        retval = itr->second->reset(measType, meanType);
    }
    else
    {
        retval = FALSE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteLayer3Filtering::remove
// PURPOSE    ::
// + remove {exponential/arithmetic} mean (all entries)
// PARAMETERS :: None
// RETURN     :: None
// **/
void LteLayer3Filtering::remove()
{
    // remove all measData
    for (MapLteMeasurementData::iterator itr = _mapMeasData.begin();
        itr != _mapMeasData.end();
        itr++)
    {
        // remove all measValue
        itr->second->remove();
    }

    _mapMeasData.clear();
}

// /**
// FUNCTION   :: LteLayer3Filtering::remove
// PURPOSE    ::
// + remove {exponential/arithmetic} mean (specific RNTI)
// PARAMETERS ::
// + LteRnti : : oppositeRnti
// RETURN     :: BOOL : TURE(remove success) / FALSE(target not found)
// **/
BOOL LteLayer3Filtering::remove(
    const LteRnti& oppositeRnti)
{
    BOOL retval = TRUE;

    // find specified measData
    MapLteMeasurementData::iterator itr = _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        // remove all measValue of found measData
        itr->second->remove();
        _mapMeasData.erase(itr);
        retval = TRUE;
    }
    else
    {
        retval = FALSE;
    }

    return retval;
}

// /**
// FUNCTION   :: LteLayer3Filtering::remove
// PURPOSE    ::
// + remove {exponential/arithmetic} mean (specific RNTI, MeasurementType)
// PARAMETERS ::
// + LteRnti : : oppositeRnti
// + LteMeasurementType : : measType
// RETURN     :: BOOL : TURE(remove success) / FALSE(target not found)
// **/
BOOL LteLayer3Filtering::remove(
    const LteRnti& oppositeRnti,
    const LteMeasurementType measType)
{
    BOOL retval = TRUE;

    // find specified measData
    MapLteMeasurementData::iterator itr = _mapMeasData.find(oppositeRnti);

    if (itr != _mapMeasData.end())
    {
        // remove specified measValue of found measData
        retval = itr->second->remove(measType);
    }
    else
    {
        retval = FALSE;
    }

    return retval;
}



// /**
// FUNCTION   :: LteLayer3Filtering::clearHOMeasIntraFreq
// PURPOSE    :: clear RNTI table of measured cells of intra-frequency
// + clear frequency information (for intra-freq)
// PARAMETERS ::
// RETURN     :: void
// **/
void  LteLayer3Filtering::clearHOMeasIntraFreq(int servingCellCh)
{
    // select a measurement type (RSRP_FOR_HO / RSRQ_FOR_HO)
    for (MapLteMeasurementTypeFreq::iterator it1 = _mapMeasFreqInfo.begin();
        it1 != _mapMeasFreqInfo.end(); it1++)
    {
        // select a frequency (channel)
        for (MapLteMeasurementFreqRnti::iterator it2 = it1->second.begin();
            it2 != it1->second.end(); it2++)
        {
            // skip DL channel of not serving cell
            if (it2->first != servingCellCh) continue;

            // select a RNTI
            for (SetLteRnti::iterator it3 = it2->second.begin();
                it3 != it2->second.end(); it3++)
            {
                remove(*it3, it1->first);
            }

            it2->second.clear();
        }
        it1->second.clear();
    }
    _mapMeasFreqInfo.clear();
}

// /**
// FUNCTION   :: LteLayer3Filtering::clearHOMeasInterFreq
// PURPOSE    :: clear RNTI table of measured cells of inter-frequency
// + clear frequency information (for inter-freq)
// PARAMETERS ::
// RETURN     :: void
// **/
void  LteLayer3Filtering::clearHOMeasInterFreq(int servingCellCh)
{
    // select a measurement type (RSRP_FOR_HO / RSRQ_FOR_HO)
    for (MapLteMeasurementTypeFreq::iterator it1 = _mapMeasFreqInfo.begin();
        it1 != _mapMeasFreqInfo.end(); it1++)
    {
        // select a frequency (channel)
        for (MapLteMeasurementFreqRnti::iterator it2 = it1->second.begin();
            it2 != it1->second.end(); it2++)
        {
            // skip DL channel of serving cell
            if (it2->first == servingCellCh) continue;

            // select a RNTI
            for (SetLteRnti::iterator it3 = it2->second.begin();
                it3 != it2->second.end(); it3++)
            {
                remove(*it3, it1->first);
            }

            it2->second.clear();
        }
        it1->second.clear();
    }
    _mapMeasFreqInfo.clear();
}

// /**
// FUNCTION   :: LteLayer3Filtering::getRntiList
// PURPOSE    :: get RNTI list of specified measurement type and channel
// + get frequency map
// PARAMETERS ::
// + [in] LteMeasurementType : : measType
// + [in] int : : channelIndex
// RETURN     :: SetLteRnti : RNTi list
// **/
SetLteRnti* LteLayer3Filtering::getRntiList(
    const LteMeasurementType measType,
    int channelIndex)
{
    MapLteMeasurementTypeFreq::iterator it1 = _mapMeasFreqInfo.find(measType);
    if (it1 == _mapMeasFreqInfo.end())
    {
        return NULL;
    }
    
    MapLteMeasurementFreqRnti* freqRntiMap = &(it1->second);
        
    MapLteMeasurementFreqRnti::iterator it2 = freqRntiMap->find(channelIndex);
    if (it2 == freqRntiMap->end())
    {
        return NULL;
    }

    return &it2->second;
}

// /**
// FUNCTION   :: LteLayer3Filtering::registerFreqInfo
// PURPOSE    :: register RNTI to table
// + register frequency information
// PARAMETERS ::
// + [in] LteMeasurementType : : measType
// + [in] int : : channelIndex
// + [in] ListLteRnti : : rnti
// RETURN     :: void
// **/
void LteLayer3Filtering::registerFreqInfo(
            LteMeasurementType measType,
            int channelIndex,
            LteRnti* rnti)
{
    // retrieve the entry of specified measurement type
    MapLteMeasurementTypeFreq::iterator it1 = _mapMeasFreqInfo.find(measType);
    if (it1 == _mapMeasFreqInfo.end())
    {
        MapLteMeasurementFreqRnti newEntry;
        _mapMeasFreqInfo.insert(MapLteMeasurementTypeFreq::value_type(
            measType, newEntry));
        it1 = _mapMeasFreqInfo.find(measType);
    }

    // retrieve the entry of specified channel
    MapLteMeasurementFreqRnti* freqRntiMap = &(it1->second);
    MapLteMeasurementFreqRnti::iterator it2 = freqRntiMap->find(channelIndex);
    if (it2 == freqRntiMap->end())
    {
        SetLteRnti newEntry;
        freqRntiMap->insert(MapLteMeasurementFreqRnti::value_type(
            channelIndex, newEntry));
        it2 = freqRntiMap->find(channelIndex);
    }
    SetLteRnti* rntiSet = &(it2->second);

    if (rntiSet->find(*rnti) == rntiSet->end())
    {
        LteRnti newRnti(rnti->nodeId, rnti->interfaceIndex);
        // register
        rntiSet->insert(newRnti);
    }
}

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
BOOL LteLayer3Filtering::getAverage(
    const int nodeId,
    const LteMeasurementType measType,
    double* value) const
{
    double sumValue = 0.0;
    int count = 0;
    BOOL ret = FALSE;
    MapLteMeasurementTypeFreq::const_iterator measFreqRntiItr =
        _mapMeasFreqInfo.find(measType);
    if (measFreqRntiItr == _mapMeasFreqInfo.end())
    {
        *value = 0.0;
        return FALSE;
    }
    const MapLteMeasurementFreqRnti& measFreqRnti =
        _mapMeasFreqInfo.find(measType)->second;
    MapLteMeasurementFreqRnti::const_iterator itr1;
    for (itr1 = measFreqRnti.begin();
        itr1 != measFreqRnti.end();
        ++itr1)
    {
        const SetLteRnti& setLteRnti = itr1->second;
        SetLteRnti::const_iterator itr2;
        for (itr2 = setLteRnti.begin();
            itr2 != setLteRnti.end();
            ++itr2)
        {
            const LteRnti& oppositeRnti = *itr2;
            if (oppositeRnti.nodeId == nodeId)
            {
                double curValue = 0.0;
                BOOL isGot =
                    get(oppositeRnti, measType, &curValue, EXPONENTIAL_MEAN);
                if (isGot == TRUE)
                {
                    sumValue += NON_DB(curValue);
                    count++;
                }
            }
        }
    }
    if (count > 0)
    {
        ret = TRUE;
        *value = IN_DB(sumValue / count);
    }
    else
    {
        ret = FALSE;
        *value = 0.0;
    }
    return ret;
}
#endif
#endif
