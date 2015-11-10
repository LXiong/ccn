#ifndef LAYER3_LTE_FILTERING_H
#define LAYER3_LTE_FILTERING_H

#include <map>
#include <math.h>

// /**
// ENUM      :: LteMeanType
// DESCRIPTION ::
// **/
typedef enum
{
    EXPONENTIAL_MEAN = 0,
    ARITHMETIC_MEAN,
    MEAN_TYPE_NUM // must be last
} LteMeanType;

// /**
// ENUM      :: LteMeasurementType
// DESCRIPTION ::
// **/
typedef enum enum_lte_measurement_type
{
    RSRP = 0,
    RSRQ,
    LTE_LIB_UL_PATHLOSS_RX0,
    LTE_LIB_UL_PATHLOSS_RX1,
    LTE_LIB_DL_PATHLOSS_RX0,
    LTE_LIB_PF_AVERAGE_THROUGHPUT_DL,
    LTE_LIB_PF_AVERAGE_THROUGHPUT_UL,
    HETERO_RSSI,
    RSRP_FOR_HO,  // for handover
    RSRQ_FOR_HO,  // for handover
    MEASUREMENT_TYPE_NUM // must be last
} LteMeasurementType;

class LteMeasurementData;
class LteMeasurementValue;
typedef struct struct_layer2_data_lte Layer2DataLte;

typedef std::map < LteRnti, LteMeasurementData* > MapLteMeasurementData;
typedef std::map < LteMeasurementType, LteMeasurementValue* >
                                                    MapLteMeasurementValue;

typedef std::set<LteRnti>                       SetLteRnti;
typedef std::map<int, SetLteRnti>               MapLteMeasurementFreqRnti;
typedef std::map<LteMeasurementType, MapLteMeasurementFreqRnti>
                                                MapLteMeasurementTypeFreq;

// /**
// CLASS       :: LteExponentialMean
// DESCRIPTION :: Calculate exponential mean
// **/
class LteExponentialMean
{
public:
    LteExponentialMean()
        :_filteredValue(0.0),
         _filterCoefficient(0.0),
         _resetFlg(true){};

    ~LteExponentialMean(){};

    void update(
        double value, double filterCoefficient, bool updateFilterCoef)
    {
        if (updateFilterCoef)
            _filterCoefficient = filterCoefficient;

        if (_resetFlg == true) {
            _filteredValue = value;
            _resetFlg = false;
        } else {
            double a = 1.0 / pow(2, (_filterCoefficient / 4.0));
            // update filteredValue
            _filteredValue = (1 - a) * _filteredValue + a * value;
        }
    };

    void get(double* value)
    {
        *value = _filteredValue;
    };

    void reset()
    {
        _filteredValue = 0.0;
        _resetFlg = true;
    };

private:

public:

private:
    double _filteredValue;
    double _filterCoefficient;
    bool _resetFlg;
};

// /**
// CLASS       :: LteArithmeticMean
// DESCRIPTION :: Calculate arithmetic mean
// **/
class LteArithmeticMean
{
public:
    LteArithmeticMean()
        :_sum(0.0),
         _count(0){};

    ~LteArithmeticMean(){};

    void update(double value)
    {
        _sum += value;
        _count++;
    };

    void get(double* value)
    {
        if (_count > 0)
            *value = _sum / _count;
        else
            *value = 0.0;
    };

    void reset()
    {
        _sum = 0.0;
        _count = 0;
    };

private:

public:

private:
    double _sum;
    UInt32 _count;
};

// /**
// CLASS       :: LteMeasurementValue
// DESCRIPTION :: Management {exponential/arithmetic} mean
// **/
class LteMeasurementValue
{
public:
    LteMeasurementValue();

    ~LteMeasurementValue();

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
    void update(double value, double filterCoef, bool updateFilterCoef);

    // /**
    // FUNCTION   :: LteMeasurementValue::get
    // PURPOSE    ::
    // + get {exponential/arithmetic} mean value
    // PARAMETERS ::
    // + [out] double* : : value
    // + [in] LteMeanType : : meanType
    // RETURN     :: BOOL : TURE(get success) / FALSE(value not found)
    // **/
    BOOL get(double* value, LteMeanType meanType);

    // /**
    // FUNCTION   :: LteMeasurementValue::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean (all entries)
    // PARAMETERS :: None
    // RETURN     :: None
    // **/
    void reset();

    // /**
    // FUNCTION   :: LteMeasurementValue::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean
    //         (specific MeasurementType, MeanType)
    // PARAMETERS ::
    // + LteMeanType : : meanType
    // RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
    // **/
    BOOL reset(LteMeanType meanType);

private:

public:

private:
    LteExponentialMean _exponentialMean;
    LteArithmeticMean _arithmeticMean;
};

// /**
// CLASS       :: LteMeasurementData
// DESCRIPTION :: Management measurement values
// **/
class LteMeasurementData
{
public:
    LteMeasurementData(const Layer2DataLte* layer2DateLte);

    ~LteMeasurementData();

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
    BOOL update(
        LteMeasurementType measType,
        double value,
        double filterCoef,
        bool updateFilterCoef);

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
    BOOL get(
        LteMeasurementType measType,
        double* value,
        LteMeanType meanType);

    // /**
    // FUNCTION   :: LteMeasurementData::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean (all entries)
    // PARAMETERS :: None
    // RETURN     :: None
    // **/
    void reset();

    // /**
    // FUNCTION   :: LteMeasurementData::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean (specific MeasurementType)
    // PARAMETERS ::
    // + LteMeasurementType : : measType
    // RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
    // **/
    BOOL reset(LteMeasurementType measType);

    // /**
    // FUNCTION   :: LteMeasurementData::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean
    //         (specific MeasurementType, MeanType)
    // PARAMETERS ::
    // + LteMeasurementType : : measType
    // + LteMeanType : : meanType
    // RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
    // **/
    BOOL reset(
        LteMeasurementType measType,
        LteMeanType meanType);

    // /**
    // FUNCTION   :: LteMeasurementData::remove
    // PURPOSE    ::
    // + remove {exponential/arithmetic} mean (all entries)
    // PARAMETERS :: None
    // RETURN     :: None
    // **/
    void remove();

    // /**
    // FUNCTION   :: LteMeasurementData::remove
    // PURPOSE    ::
    // + remove {exponential/arithmetic} mean (specific MeasurementType)
    // PARAMETERS ::
    // + LteMeasurementType : : measType
    // RETURN     :: BOOL : TURE(remove success) / FALSE(target not found)
    // **/
    BOOL remove(LteMeasurementType measType);

private:

private:
    MapLteMeasurementValue _mapMeasValue;
    const Layer2DataLte* _layer2DateLte;
};

// /**
// CLASS       :: LteLayer3Filtering
// DESCRIPTION :: Management measurement datas
// **/
class LteLayer3Filtering
{
public:
    LteLayer3Filtering(const Layer2DataLte* layer2DateLte);

    virtual ~LteLayer3Filtering();

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
    BOOL update(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType,
        const double value);

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
    BOOL update(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType,
        const double value,
        double filterCoef);

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
    BOOL get(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType,
        double* value) const;

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
    BOOL get(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType,
        double* value,
        const LteMeanType meanType) const;

    // /**
    // FUNCTION   :: LteLayer3Filtering::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean (all entries)
    // PARAMETERS :: None
    // RETURN     :: None
    // **/
    void reset();

    // /**
    // FUNCTION   :: LteLayer3Filtering::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean (specific RNTI)
    // PARAMETERS ::
    // + LteRnti : : oppositeRnti
    // RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
    // **/
    BOOL reset(
        const LteRnti& oppositeRnti);

    // /**
    // FUNCTION   :: LteLayer3Filtering::reset
    // PURPOSE    ::
    // + reset {exponential/arithmetic} mean (specific RNTI, MeasurementType)
    // PARAMETERS ::
    // + LteRnti : : oppositeRnti
    // + LteMeasurementType : : measType
    // RETURN     :: BOOL : TURE(reset success) / FALSE(target not found)
    // **/
    BOOL reset(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType);

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
    BOOL reset(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType,
        const LteMeanType meanType);

    // /**
    // FUNCTION   :: LteLayer3Filtering::remove
    // PURPOSE    ::
    // + remove {exponential/arithmetic} mean (all entries)
    // PARAMETERS :: None
    // RETURN     :: None
    // **/
    void remove();

    // /**
    // FUNCTION   :: LteLayer3Filtering::remove
    // PURPOSE    ::
    // + remove {exponential/arithmetic} mean (specific RNTI)
    // PARAMETERS ::
    // + LteRnti : : oppositeRnti
    // RETURN     :: BOOL : TURE(remove success) / FALSE(target not found)
    // **/
    BOOL remove(
        const LteRnti& oppositeRnti);

    // /**
    // FUNCTION   :: LteLayer3Filtering::remove
    // PURPOSE    ::
    // + remove {exponential/arithmetic} mean
    //          (specific RNTI, MeasurementType)
    // PARAMETERS ::
    // + LteRnti : : oppositeRnti
    // + LteMeasurementType : : measType
    // RETURN     :: BOOL : TURE(remove success) / FALSE(target not found)
    // **/
    BOOL remove(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType);

private:
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
    BOOL updateExec(
        const LteRnti& oppositeRnti,
        const LteMeasurementType measType,
        double value,
        double filterCoef,
        bool updateFilterCoef);

public:

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
public:
    BOOL getAverage(
        const int nodeId,
        const LteMeasurementType measType,
        double* value) const;
#endif
#endif

private:
    MapLteMeasurementData _mapMeasData;
    const Layer2DataLte* _layer2DateLte;


public:
    // /**
    // FUNCTION   :: LteLayer3Filtering::clearHOMeasIntraFreq
    // PURPOSE    ::
    // + clear frequency information (for intra-freq)
    // PARAMETERS ::
    // RETURN     :: void
    // **/
    void  clearHOMeasIntraFreq(int servingCellCh);

    // /**
    // FUNCTION   :: LteLayer3Filtering::clearHOMeasInterFreq
    // PURPOSE    ::
    // + clear frequency information (for inter-freq)
    // PARAMETERS ::
    // RETURN     :: void
    // **/
    void  clearHOMeasInterFreq(int servingCellCh);

    // /**
    // FUNCTION   :: LteLayer3Filtering::getRntiList
    // PURPOSE    ::
    // + get frequency map
    // PARAMETERS ::
    // + [in] LteMeasurementType : : measType
    // + [in] int : : channelIndex
    // RETURN     :: SetLteRnti : RNTi list
    // **/
    SetLteRnti* getRntiList(
        const LteMeasurementType measType,
        int channelIndex);

    // /**
    // FUNCTION   :: LteLayer3Filtering::registerFreqInfo
    // PURPOSE    ::
    // + register frequency information
    // PARAMETERS ::
    // + [in] LteMeasurementType : : measType
    // + [in] int : : channelIndex
    // + [in] LteRnti : : rnti
    // RETURN     :: void
    // **/
    void registerFreqInfo(
                LteMeasurementType measType,
                int channelIndex,
                LteRnti* rnti);
private:
    // to manage RNTI list of cells measured
    MapLteMeasurementTypeFreq _mapMeasFreqInfo;

};

#endif // LAYER3_LTE_FILTERING_H
