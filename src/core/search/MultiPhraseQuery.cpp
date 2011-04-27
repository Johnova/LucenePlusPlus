/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2011 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "LuceneInc.h"
#include "MultiPhraseQuery.h"
#include "_MultiPhraseQuery.h"
#include "Searcher.h"
#include "Term.h"
#include "TermQuery.h"
#include "MultipleTermPositions.h"
#include "ExactPhraseScorer.h"
#include "SloppyPhraseScorer.h"
#include "Similarity.h"
#include "IndexReader.h"
#include "ComplexExplanation.h"
#include "BooleanQuery.h"
#include "PhraseQuery.h"
#include "MiscUtils.h"
#include "StringUtils.h"

namespace Lucene
{
    MultiPhraseQuery::MultiPhraseQuery()
    {
        termArrays = Collection< Collection<TermPtr> >::newInstance();
        positions = Collection<int32_t>::newInstance();
        slop = 0;
    }
    
    MultiPhraseQuery::~MultiPhraseQuery()
    {
    }
    
    void MultiPhraseQuery::setSlop(int32_t s)
    {
        slop = s;
    }
    
    int32_t MultiPhraseQuery::getSlop()
    {
        return slop;
    }
    
    void MultiPhraseQuery::add(TermPtr term)
    {
        add(newCollection<TermPtr>(term));
    }
    
    void MultiPhraseQuery::add(Collection<TermPtr> terms)
    {
        int32_t position = 0;
        if (!positions.empty())
            position = positions[positions.size() - 1] + 1;
        add(terms, position);
    }
    
    void MultiPhraseQuery::add(Collection<TermPtr> terms, int32_t position)
    {
        if (termArrays.empty())
            field = terms[0]->field();
        for (Collection<TermPtr>::iterator term = terms.begin(); term != terms.end(); ++term)
        {
            if ((*term)->field() != field)
                boost::throw_exception(IllegalArgumentException(L"All phrase terms must be in the same field (" + field + L"): " + (*term)->toString()));
        }
        termArrays.add(terms);
        positions.add(position);
    }
    
    Collection< Collection<TermPtr> > MultiPhraseQuery::getTermArrays()
    {
        return termArrays;
    }
    
    Collection<int32_t> MultiPhraseQuery::getPositions()
    {
        return positions;
    }
    
    void MultiPhraseQuery::extractTerms(SetTerm terms)
    {
        for (Collection< Collection<TermPtr> >::iterator arr = termArrays.begin(); arr != termArrays.end(); ++arr)
        {
            for (Collection<TermPtr>::iterator term = arr->begin(); term != arr->end(); ++term)
                terms.add(*term);
        }
    }
    
    QueryPtr MultiPhraseQuery::rewrite(IndexReaderPtr reader)
    {
        if (termArrays.size() == 1) // optimize one-term case
        {
            Collection<TermPtr> terms(termArrays[0]);
            BooleanQueryPtr boq(newLucene<BooleanQuery>(true));
            for (Collection<TermPtr>::iterator term = terms.begin(); term != terms.end(); ++term)
                boq->add(newLucene<TermQuery>(*term), BooleanClause::SHOULD);
            boq->setBoost(getBoost());
            return boq;
        }
        else
            return shared_from_this();
    }
    
    WeightPtr MultiPhraseQuery::createWeight(SearcherPtr searcher)
    {
        return newLucene<MultiPhraseWeight>(shared_from_this(), searcher);
    }
    
    String MultiPhraseQuery::toString(const String& field)
    {
        StringStream buffer;
        if (field.empty() || this->field != field)
            buffer << this->field << L":";
        buffer << L"\"";
        for (Collection< Collection<TermPtr> >::iterator arr = termArrays.begin(); arr != termArrays.end(); ++arr)
        {
            if (arr != termArrays.begin())
                buffer << L" ";
            if (arr->size() > 1)
            {
                buffer << L"(";
                for (Collection<TermPtr>::iterator term = arr->begin(); term != arr->end(); ++term)
                {
                    if (term != arr->begin())
                        buffer << L" ";
                    buffer << (*term)->text();
                }
                buffer << L")";
            }
            else if (!arr->empty())
                buffer << (*arr)[0]->text();
        }
        buffer << L"\"";
        
        if (slop != 0)
            buffer << L"~" << slop;
        
        buffer << boostString();
        
        return buffer.str();
    }
    
    bool MultiPhraseQuery::equals(LuceneObjectPtr other)
    {
        if (LuceneObject::equals(other))
            return true;
        
        MultiPhraseQueryPtr otherMultiPhraseQuery(boost::dynamic_pointer_cast<MultiPhraseQuery>(other));
        if (!otherMultiPhraseQuery)
            return false;
        
        return (getBoost() == otherMultiPhraseQuery->getBoost() && slop == otherMultiPhraseQuery->slop &&
                termArraysEquals(termArrays, otherMultiPhraseQuery->termArrays) &&
                positions.equals(otherMultiPhraseQuery->positions));
    }
    
    int32_t MultiPhraseQuery::hashCode()
    {
        return MiscUtils::doubleToIntBits(getBoost()) ^ slop ^  termArraysHashCode() ^ MiscUtils::hashCode(positions.begin(), positions.end(), MiscUtils::hashNumeric<int32_t>) ^ 0x4ac65113;
    }
        
    int32_t MultiPhraseQuery::termArraysHashCode()
    {
        int32_t hashCode = 1;
        for (Collection< Collection<TermPtr> >::iterator arr = termArrays.begin(); arr != termArrays.end(); ++arr)
            hashCode = 31 * hashCode + MiscUtils::hashCode(arr->begin(), arr->end(), MiscUtils::hashLucene<TermPtr>);
        return hashCode;
    }
    
    struct equalTermArrays
    {
        inline bool operator()(const Collection<TermPtr>& first, const Collection<TermPtr>& second) const
        {
            if (first.size() != second.size())
                return false;
            return first.equals(second, luceneEquals<TermPtr>());
        }
    };
    
    bool MultiPhraseQuery::termArraysEquals(Collection< Collection<TermPtr> > first, Collection< Collection<TermPtr> > second)
    {
        return first.equals(second, equalTermArrays());
    }
    
    LuceneObjectPtr MultiPhraseQuery::clone(LuceneObjectPtr other)
    {
        LuceneObjectPtr clone = other ? other : newLucene<MultiPhraseQuery>();
        MultiPhraseQueryPtr cloneQuery(boost::static_pointer_cast<MultiPhraseQuery>(Query::clone(clone)));
        cloneQuery->field = field;
        cloneQuery->termArrays = termArrays;
        cloneQuery->positions = positions;
        cloneQuery->slop = slop;
        return cloneQuery;
    }
    
    MultiPhraseWeight::MultiPhraseWeight(MultiPhraseQueryPtr query, SearcherPtr searcher)
    {
        this->query = query;
        this->similarity = query->getSimilarity(searcher);
        this->value = 0.0;
        this->idf = 0.0;
        this->queryNorm = 0.0;
        this->queryWeight = 0.0;
        
        // compute idf
        Collection<TermPtr> allTerms(Collection<TermPtr>::newInstance());
        for (Collection< Collection<TermPtr> >::iterator arr = query->termArrays.begin(); arr != query->termArrays.end(); ++arr)
        {
            for (Collection<TermPtr>::iterator term = arr->begin(); term != arr->end(); ++term)
                allTerms.add(*term);
        }
        idfExp = similarity->idfExplain(allTerms, searcher);
        idf = idfExp->getIdf();
    }
    
    MultiPhraseWeight::~MultiPhraseWeight()
    {
    }
    
    QueryPtr MultiPhraseWeight::getQuery()
    {
        return query;
    }
    
    double MultiPhraseWeight::getValue()
    {
        return value;
    }
    
    double MultiPhraseWeight::sumOfSquaredWeights()
    {
        queryWeight = idf * getQuery()->getBoost(); // compute query weight
        return queryWeight * queryWeight; // square it
    }
    
    void MultiPhraseWeight::normalize(double norm)
    {
        queryNorm = norm;
        queryWeight *= queryNorm; // normalize query weight
        value = queryWeight * idf; // idf for document 
    }
    
    ScorerPtr MultiPhraseWeight::scorer(IndexReaderPtr reader, bool scoreDocsInOrder, bool topScorer)
    {
        if (query->termArrays.empty()) // optimize zero-term case
            return ScorerPtr();
        
        Collection<PostingsAndFreqPtr> postingsFreqs(Collection<PostingsAndFreqPtr>::newInstance(query->termArrays.size()));

        for (int32_t pos = 0; pos < postingsFreqs.size(); ++pos)
        {
            Collection<TermPtr> terms(query->termArrays[pos]);

            TermPositionsPtr p;
            int32_t docFreq = 0;

            if (terms.size() > 1)
            {
                p = newLucene<MultipleTermPositions>(reader, terms);

                // coarse -- this over counts since a given doc can have more than one terms
                docFreq = 0;
                for (int32_t termIdx = 0; termIdx < terms.size(); ++termIdx)
                    docFreq += reader->docFreq(terms[termIdx]);
            }
            else
            {
                p = reader->termPositions(terms[0]);
                docFreq = reader->docFreq(terms[0]);

                if (!p)
                    return ScorerPtr();
            }

            postingsFreqs[pos] = newLucene<PostingsAndFreq>(p, docFreq, query->positions[pos]);
        }

        // sort by increasing docFreq order
        if (query->slop == 0)
            std::sort(postingsFreqs.begin(), postingsFreqs.end(), luceneCompare<PostingsAndFreqPtr>());
        
        if (query->slop == 0)
        {
            ExactPhraseScorerPtr s(newLucene<ExactPhraseScorer>(shared_from_this(), postingsFreqs, similarity, reader->norms(query->field)));
            if (s->noDocs)
                return ScorerPtr();
            else
                return s;
        }
        else
            return newLucene<SloppyPhraseScorer>(shared_from_this(), postingsFreqs, similarity, query->slop, reader->norms(query->field));
    }
    
    ExplanationPtr MultiPhraseWeight::explain(IndexReaderPtr reader, int32_t doc)
    {
        ComplexExplanationPtr result(newLucene<ComplexExplanation>());
        result->setDescription(L"weight(" + query->toString() + L" in " + StringUtils::toString(doc) + L"), product of:");
        
        ExplanationPtr idfExpl(newLucene<Explanation>(idf, L"idf(" + query->field + L":" + idfExp->explain() + L")"));
        
        // explain query weight
        ExplanationPtr queryExpl(newLucene<Explanation>());
        queryExpl->setDescription(L"queryWeight(" + query->toString() + L"), product of:");
        
        ExplanationPtr boostExpl(newLucene<Explanation>(query->getBoost(), L"boost"));
        if (query->getBoost() != 1.0)
            queryExpl->addDetail(boostExpl);
        
        queryExpl->addDetail(idfExpl);
        
        ExplanationPtr queryNormExpl(newLucene<Explanation>(queryNorm, L"queryNorm"));
        queryExpl->addDetail(queryNormExpl);
        
        queryExpl->setValue(boostExpl->getValue() * idfExpl->getValue() * queryNormExpl->getValue());
        result->addDetail(queryExpl);
        
        // explain field weight
        ComplexExplanationPtr fieldExpl(newLucene<ComplexExplanation>());
        fieldExpl->setDescription(L"fieldWeight(" + query->toString() + L" in " + StringUtils::toString(doc) + L"), product of:");
        
        ScorerPtr _scorer(scorer(reader, true, false));
        if (!_scorer)
            return newLucene<Explanation>(0.0, L"no matching docs");
            
        ExplanationPtr tfExplanation(newLucene<Explanation>());
        int32_t d = _scorer->advance(doc);
        double phraseFreq = d == doc ? _scorer->freq() : 0.0;
        tfExplanation->setValue(similarity->tf(phraseFreq));
        tfExplanation->setDescription(L"tf(phraseFreq=" + StringUtils::toString(phraseFreq) + L")");
        
        fieldExpl->addDetail(tfExplanation);
        fieldExpl->addDetail(idfExpl);
        
        ExplanationPtr fieldNormExpl(newLucene<Explanation>());
        ByteArray fieldNorms(reader->norms(query->field));
        double fieldNorm = fieldNorms ? similarity->decodeNormValue(fieldNorms[doc]) : 1.0;
        fieldNormExpl->setValue(fieldNorm);
        fieldNormExpl->setDescription(L"fieldNorm(field=" + query->field + L", doc=" + StringUtils::toString(doc) + L")");
        fieldExpl->addDetail(fieldNormExpl);
        
        fieldExpl->setMatch(tfExplanation->isMatch());
        fieldExpl->setValue(tfExplanation->getValue() * idfExpl->getValue() * fieldNormExpl->getValue());
        
        result->addDetail(fieldExpl);
        result->setMatch(fieldExpl->getMatch());
        
        // combine them
        result->setValue(queryExpl->getValue() * fieldExpl->getValue());
        
        if (queryExpl->getValue() == 1.0)
            return fieldExpl;
        
        return result;
    }
}
