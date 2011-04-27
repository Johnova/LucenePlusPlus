/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2011 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "LuceneInc.h"
#include "MaxPayloadFunction.h"
#include "Explanation.h"
#include "MiscUtils.h"
#include "StringUtils.h"

namespace Lucene
{
    MaxPayloadFunction::~MaxPayloadFunction()
    {
    }
    
    double MaxPayloadFunction::currentScore(int32_t docId, const String& field, int32_t start, int32_t end,
                                                int32_t numPayloadsSeen, double currentScore, double currentPayloadScore)
    {
        if (numPayloadsSeen == 0)
            return currentPayloadScore;
        else
            return std::max(currentPayloadScore, currentScore);
    }
    
    double MaxPayloadFunction::docScore(int32_t docId, const String& field, int32_t numPayloadsSeen, double payloadScore)
    {
        return numPayloadsSeen > 0 ? payloadScore : 1.0;
    }
    
    ExplanationPtr MaxPayloadFunction::explain(int32_t docId, int32_t numPayloadsSeen, double payloadScore)
    {
        ExplanationPtr expl(newLucene<Explanation>());
        double maxPayloadScore = (numPayloadsSeen > 0 ? payloadScore : 1);
        expl->setValue(maxPayloadScore);
        expl->setDescription(L"MaxPayloadFunction(...)");
        return expl;
    }
    
    int32_t MaxPayloadFunction::hashCode()
    {
        int32_t prime = 31;
        int32_t result = 1;
        result = prime * result + StringUtils::hashCode(getClassName());
        return result;
    }
    
    bool MaxPayloadFunction::equals(LuceneObjectPtr other)
    {
        if (LuceneObject::equals(other))
            return true;
        if (!other)
            return false;
        if (!MiscUtils::equalTypes(shared_from_this(), other))
            return false;
        return true;
    }
}
