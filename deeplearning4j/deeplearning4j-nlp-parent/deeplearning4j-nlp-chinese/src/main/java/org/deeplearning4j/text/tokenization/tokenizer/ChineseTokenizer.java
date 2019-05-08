/*******************************************************************************
 * Copyright (c) 2015-2019 Skymind, Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

package org.deeplearning4j.text.tokenization.tokenizer;

import org.ansj.domain.Result;
import org.ansj.domain.Term;
import org.ansj.splitWord.analysis.NlpAnalysis;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;


public class ChineseTokenizer implements Tokenizer {

    private TokenPreProcess tokenPreProcess;
    private List<Term> tokenList;
    private Iterator<Term> tokenIter;

    public ChineseTokenizer() {}

    public ChineseTokenizer(String toTokenize) {
        Result result = NlpAnalysis.parse(toTokenize);
        this.tokenList = result.getTerms();
        this.tokenIter = tokenList.iterator();
    }

    @Override
    public boolean hasMoreTokens() {
        return tokenIter.hasNext();
    }

    @Override
    public int countTokens() {
        return tokenList != null ? tokenList.size() : 0;
    }

    @Override
    public String nextToken() {
        if (!hasMoreTokens()) {
            throw new NoSuchElementException();
        }
        return this.tokenPreProcess != null ? this.tokenPreProcess.preProcess(tokenIter.next().getName())
                        : tokenIter.next().getName();
    }

    @Override
    public List<String> getTokens() {
        ArrayList tokenList = new ArrayList();

        while (hasMoreTokens()) {
            tokenList.add(nextToken());
        }
        return tokenList;
    }

    @Override
    public void setTokenPreProcessor(TokenPreProcess tokenPreProcessor) {
        this.tokenPreProcess = tokenPreProcessor;
    }

}
