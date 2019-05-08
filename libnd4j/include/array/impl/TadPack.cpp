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

//
//  @author raver119@gmail.com
//

#include "../TadPack.h"

namespace nd4j {
    TadPack::TadPack(DataBuffer &shapes, DataBuffer &offets, Nd4jLong numTads) {
        _tadShape = shapes;
        _tadOffsets = offets;
        _numTads = numTads;
    }

    Nd4jLong* TadPack::primaryShapeInfo() {
        return reinterpret_cast<Nd4jLong *>(_tadShape.primary());
    }
    Nd4jLong* TadPack::primaryOffsets() {
        return reinterpret_cast<Nd4jLong *>(_tadOffsets.primary());
    }

    Nd4jLong* TadPack::specialShapeInfo() {
        return reinterpret_cast<Nd4jLong *>(_tadShape.special());
    }

    Nd4jLong* TadPack::specialOffsets() {
        return reinterpret_cast<Nd4jLong *>(_tadOffsets.special());
    }

    Nd4jLong TadPack::numberOfTads() {
        return _numTads;
    }
}