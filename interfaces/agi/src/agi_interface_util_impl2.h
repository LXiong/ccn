#pragma once
#include "agi_interface_util_impl.h"

template<class ID, class TIME, class SIGNAL, class MAPPING>
class CAgiInterfaceUtilImpl2 : public CAgiInterfaceUtilImpl<ID, TIME, SIGNAL, MAPPING>
{
public:
    CAgiInterfaceUtilImpl2() {}
    virtual ~CAgiInterfaceUtilImpl2() {}

    virtual void Shutdown();
};


template<class ID, class TIME, class SIGNAL, class MAPPING>
void CAgiInterfaceUtilImpl2<ID, TIME, SIGNAL, MAPPING>::Shutdown()
{
    CAgiInterfaceSmartPtr<AgStkCommUtilLib::IAgStkCommUtilFactory> pFactory = GetFactory();
    if (pFactory)
    {
        CAgiInterfaceSmartPtr<AgStkCommUtilLib::IAgStkCommUtilFinalize> pFactoryFinalize;
        if (SUCCEEDED(pFactory->QueryInterface(__uuidof(AgStkCommUtilLib::IAgStkCommUtilFinalize), (void**)&pFactoryFinalize)))
        {
            pFactoryFinalize->Shutdown();
        }
    }
}