/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2011 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "LuceneInc.h"
#include "TermsHashPerThread.h"
#include "TermsHashPerField.h"
#include "DocInverterPerThread.h"
#include "TermsHash.h"
#include "TermsHashConsumer.h"
#include "TermsHashConsumerPerThread.h"
#include "CharBlockPool.h"
#include "IntBlockPool.h"
#include "DocumentsWriter.h"
#include "_DocumentsWriter.h"

namespace Lucene
{
    TermsHashPerThread::TermsHashPerThread(DocInverterPerThreadPtr docInverterPerThread, TermsHashPtr termsHash, TermsHashPtr nextTermsHash, TermsHashPerThreadPtr primaryPerThread)
    {
        this->primary = false;
        this->_docInverterPerThread = docInverterPerThread;
        this->_termsHash = termsHash;
        this->nextTermsHash = nextTermsHash;
        this->_primaryPerThread = primaryPerThread;
    }
    
    TermsHashPerThread::~TermsHashPerThread()
    {
    }
    
    void TermsHashPerThread::initialize()
    {
        DocInverterPerThreadPtr docInverterPerThread(_docInverterPerThread);
        TermsHashPtr termsHash(_termsHash);
        docState = docInverterPerThread->docState;
        consumer = termsHash->consumer->addThread(shared_from_this());
        
        if (nextTermsHash)
        {
            // We are primary
            charPool = newLucene<CharBlockPool>(DocumentsWriterPtr(termsHash->_docWriter));
            primary = true;
        }
        else
        {
            charPool = TermsHashPerThreadPtr(_primaryPerThread)->charPool;
            primary = false;
        }
        
        intPool = newLucene<IntBlockPool>(DocumentsWriterPtr(termsHash->_docWriter));
        bytePool = newLucene<ByteBlockPool>(DocumentsWriterPtr(termsHash->_docWriter)->byteBlockAllocator);

        if (nextTermsHash)
            nextPerThread = nextTermsHash->addThread(docInverterPerThread, shared_from_this());
    }
    
    InvertedDocConsumerPerFieldPtr TermsHashPerThread::addField(DocInverterPerFieldPtr docInverterPerField, FieldInfoPtr fieldInfo)
    {
        return newLucene<TermsHashPerField>(docInverterPerField, shared_from_this(), nextPerThread, fieldInfo);
    }
    
    void TermsHashPerThread::abort()
    {
        SyncLock syncLock(this);
        reset(true);
        consumer->abort();
        if (nextPerThread)
            nextPerThread->abort();
    }
    
    void TermsHashPerThread::startDocument()
    {
        consumer->startDocument();
        if (nextPerThread)
            nextPerThread->consumer->startDocument();
    }
    
    DocWriterPtr TermsHashPerThread::finishDocument()
    {
        DocWriterPtr doc(consumer->finishDocument());
        DocWriterPtr doc2(nextPerThread ? nextPerThread->consumer->finishDocument() : DocWriterPtr());
        if (!doc)
            return doc2;
        else
        {
            doc->setNext(doc2);
            return doc;
        }
    }

    void TermsHashPerThread::reset(bool recyclePostings)
    {
        intPool->reset();
        bytePool->reset();
        
        if (primary)
            charPool->reset();
    }
}
