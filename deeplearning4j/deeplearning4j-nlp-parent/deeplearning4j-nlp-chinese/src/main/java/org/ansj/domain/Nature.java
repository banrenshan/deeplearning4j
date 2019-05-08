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

package org.ansj.domain;

import org.ansj.library.NatureLibrary;

import java.io.Serializable;

/**
 * 这里面封装了一些基本的词性.
 * 
 * @author ansj
 * 
 */
public class Nature implements Serializable {
    /**
     * 
     */
    private static final long serialVersionUID = -1427092012930357598L;
    // 词性的名称
    public final String natureStr;
    // 词性对照表的位置
    public final int index;
    // 词性的下标值
    public final int natureIndex;
    // 词性的频率
    public final int allFrequency;

    public static final Nature NW = NatureLibrary.getNature("nw");

    public static final Nature NRF = NatureLibrary.getNature("nrf");

    public static final Nature NR = NatureLibrary.getNature("nr");

    public static final Nature NULL = NatureLibrary.getNature("null");

    public Nature(String natureStr, int index, int natureIndex, int allFrequency) {
        this.natureStr = natureStr;
        this.index = index;
        this.natureIndex = natureIndex;
        this.allFrequency = allFrequency;
    }

    public Nature(String natureStr) {
        this.natureStr = natureStr;
        this.index = 0;
        this.natureIndex = 0;
        this.allFrequency = 0;
    }

    @Override
    public String toString() {
        return natureStr + ":" + index + ":" + natureIndex;
    }
}
