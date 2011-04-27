/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2011 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#ifndef WORDLISTLOADER_H
#define WORDLISTLOADER_H

#include "LuceneObject.h"

namespace Lucene
{
    /// Loader for text files that represent a list of stopwords.
    class LPPAPI WordlistLoader : public LuceneObject
    {
    public:
        virtual ~WordlistLoader();
        
        LUCENE_CLASS(WordlistLoader);
    
    public:
        /// Loads a text file and adds every line as an entry to a HashSet (omitting leading and
        /// trailing whitespace). Every line of the file should contain only one word. The words
        /// need to be in lowercase if you make use of an Analyzer which uses LowerCaseFilter 
        /// (like StandardAnalyzer).
        ///
        /// @param wordfile File name containing the wordlist
        /// @param comment The comment string to ignore
        /// @return A set with the file's words
        static HashSet<String> getWordSet(const String& wordfile, const String& comment = EmptyString);
        
        /// Loads a text file and adds every line as an entry to a HashSet (omitting leading and 
        /// trailing whitespace). Every line of the file should contain only one word. The words 
        /// need to be in lowercase if you make use of an Analyzer which uses LowerCaseFilter 
        /// (like StandardAnalyzer).
        ///
        /// @param reader Reader containing the wordlist
        /// @param comment The comment string to ignore
        /// @return A set with the file's words
        static HashSet<String> getWordSet(ReaderPtr reader, const String& comment = EmptyString);
        
        /// Reads stopwords from a stopword list in Snowball format.
        ///
        /// The snowball format is the following:
        /// <ul>
        ///   <li>Lines may contain multiple words separated by whitespace.
        ///   <li>The comment character is the vertical line (|).
        ///   <li>Lines may contain trailing comments.
        /// </ul>
        ///
        /// @param reader Reader containing a Snowball stopword list
        /// @return A Set with the reader's words
        static HashSet<String> getSnowballWordSet(ReaderPtr reader);
        
        /// Reads a stem dictionary. Each line contains:
        /// <pre>word\tstem</pre>
        /// (ie. two tab separated words)
        /// @return stem dictionary that overrules the stemming algorithm
        static MapStringString getStemDict(const String& wordstemfile);
    };
}

#endif
