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
// Created by raver119 on 20.11.17.
//

#include "testlayers.h"
#include <Graph.h>
#include <chrono>
#include <Node.h>
#include <ops/declarable/CustomOperations.h>
#include <graph/profiling/GraphProfilingHelper.h>
#include <type_conversions.h>
#include <helpers/threshold.h>
#include <helpers/MmulHelper.h>
#include <ops/ops.h>
#include <OmpLaunchHelper.h>
#include <GradCheck.h>
#include <Loops.h>

#include <helpers/BenchmarkHelper.h>
#include <helpers/ConstantTadHelper.h>
#include <array>

using namespace nd4j;
using namespace nd4j::graph;

class PlaygroundTests : public testing::Test {
public:
    int numIterations = 3;
    int poolSize = 10;

    PlaygroundTests() {
        printf("\n");
        fflush(stdout);
    }
};

TEST_F(PlaygroundTests, test_small_reductions) {
    auto f = NDArrayFactory::create<float>('c', {1024 ,1024});
    f.assign(1.0f);

    int iterations = 100;
    std::vector<Nd4jLong> results(iterations);
    Nd4jLong mean = 0L;
    Nd4jLong max = 0L;
    Nd4jLong min = DataTypeUtils::max<Nd4jLong>();

    for (int e = 0; e < iterations; e++) {
        auto x = NDArrayFactory::create<float>('c', {4, 64});
        auto z = NDArrayFactory::create<float>('c', {64});
        x.assign(1.0f);
        int axis = 0;

        auto tadPack = nd4j::ConstantTadHelper::getInstance()->tadForDimensions(x.shapeInfo(), axis);

        auto timeStart = std::chrono::system_clock::now();

        NativeOpExcutioner::execReduceFloat(reduce::Mean, x.buffer(), x.shapeInfo(), nullptr, z.buffer(), z.shapeInfo(), &axis, 1, tadPack.primaryShapeInfo(), tadPack.primaryOffsets());

        auto timeEnd = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)).count();
        results[e] = duration;
        mean += duration;

        if (duration > max)
            max = duration;

        if (duration < min)
            min = duration;
    }

    mean /= iterations;
    std::sort(results.begin(), results.end());

    nd4j_printf("Median time: [%lld]; Mean time: [%lld]; Min time: [%lld]; Max time: [%lld]\n", results[results.size() / 2], mean, min, max);
}

TEST_F(PlaygroundTests, Test_PermutedArray_Operation_1) {
    auto x = NDArrayFactory::create<float>('c',{64, 32, 4, 32});
    auto z = NDArrayFactory::create<float>('c', {4, 64, 32, 32});
    x.assign(1.0f);

    x.permutei({2, 0, 3, 1});

    //x.printShapeInfo("x");

    int iterations = 100;
    std::vector<Nd4jLong> results(iterations);
    Nd4jLong mean = 0L;
    Nd4jLong max = 0L;
    Nd4jLong min = DataTypeUtils::max<Nd4jLong>();


    for (int e = 0; e < iterations; e++) {
        auto timeStart = std::chrono::system_clock::now();

        NativeOpExcutioner::execTransformStrict(transform::StrictOps::Sin, x.buffer(), x.shapeInfo(), z.buffer(), z.shapeInfo(), nullptr, nullptr, nullptr);

        auto timeEnd = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)).count();
        results[e] = duration;
        mean += duration;

        if (duration > max)
            max = duration;

        if (duration < min)
            min = duration;
    }

    mean /= iterations;
    std::sort(results.begin(), results.end());

    nd4j_printf("Median time: [%lld]; Mean time: [%lld]; Min time: [%lld]; Max time: [%lld]\n", results[results.size() / 2], mean, min, max);

}

TEST_F(PlaygroundTests, Test_PermutedArray_Operation_2) {

    //x.printShapeInfo("x");

    int iterations = 100;
    std::vector<Nd4jLong> results(iterations);
    Nd4jLong mean = 0L;
    Nd4jLong max = 0L;
    Nd4jLong min = DataTypeUtils::max<Nd4jLong>();


    for (int e = 0; e < iterations; e++) {
        Nd4jLong eShapeInfo[] = {2, 8, 256, 256, 1, 8192, 1, 99};
        Nd4jLong xShapeInfo[] = {2, 8, 256, 1024, 1, 8192, 0, 99};
        Nd4jLong yShapeInfo[] = {2, 8, 256, 256, 1, 8192, 1, 99};
        float xBuff[8*1024];

        NDArray x(xBuff, xShapeInfo);
        //NDArray x(eShapeInfo, nd4j::DataType::FLOAT32, true);
        NDArray z(yShapeInfo, nd4j::DataType::FLOAT32, true);
        x.linspace(0.1f, 0.01f);

        auto timeStart = std::chrono::system_clock::now();

        NativeOpExcutioner::execTransformStrict(transform::StrictOps::Tanh, x.buffer(), x.shapeInfo(), z.buffer(), z.shapeInfo(), nullptr, nullptr, nullptr);

        auto timeEnd = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)).count();
        results[e] = duration;
        mean += duration;

        if (duration > max)
            max = duration;

        if (duration < min)
            min = duration;
    }

    mean /= iterations;
    std::sort(results.begin(), results.end());

    nd4j_printf("Median time: [%lld]; Mean time: [%lld]; Min time: [%lld]; Max time: [%lld]\n", results[results.size() / 2], mean, min, max);

}

TEST_F(PlaygroundTests, test_reduce_3) {
    // auto x = NDArrayFactory::create<float>('c', {4096, 8192});
    // auto y = NDArrayFactory::create<float>('c', {8192});
    // auto z = NDArrayFactory::create<float>('c', {4096});

    auto x = NDArrayFactory::create<float>('c', {2048, 4096});
    auto y = NDArrayFactory::create<float>('c', {4096});
    auto z = NDArrayFactory::create<float>('c', {2048});

    auto dim = NDArrayFactory::create<int>('c', {1}, {1});
    auto iterations = 100;
    std::vector<Nd4jLong> results(iterations);
    Nd4jLong mean = 0L;
    Nd4jLong max = 0L;
    Nd4jLong min = DataTypeUtils::max<Nd4jLong>();

    NativeOps nativeOps;

    for (int e = 0; e < iterations; e++) {
        auto timeStart = std::chrono::system_clock::now();

        nativeOps.execReduce3(nullptr, reduce3::CosineDistance, x.buffer(), x.shapeInfo(), x.specialBuffer(),
                              x.specialShapeInfo(), nullptr, y.buffer(), y.shapeInfo(), y.specialBuffer(),
                              y.specialShapeInfo(), z.buffer(), z.shapeInfo(), z.specialBuffer(), z.specialShapeInfo(),
                              dim.buffer(), dim.shapeInfo(), dim.specialBuffer(), dim.specialShapeInfo(), nullptr,
                              nullptr, nullptr, nullptr);

        auto timeEnd = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)).count();
        results[e] = duration;
        mean += duration;

        if (duration > max)
            max = duration;

        if (duration < min)
            min = duration;
    }

    mean /= iterations;
    std::sort(results.begin(), results.end());

    nd4j_printf("Median time: [%lld]; Mean time: [%lld]; Min time: [%lld]; Max time: [%lld]\n", results[results.size() / 2], mean, min, max);
}

/*
TEST_F(PlaygroundTests, Test_OpBenchmark_1) {

    BenchmarkHelper helper;

    ScalarBenchmark sb1(scalar::Add, "add", NDArrayFactory::create_<float>('c', {100, 100}), NDArrayFactory::create_<float>(1.0f), NDArrayFactory::create_<float>('c', {100, 100}));
    ScalarBenchmark sb2(scalar::Add, "add", NDArrayFactory::create_<float>('c', {1000, 1000}), NDArrayFactory::create_<float>(1.0f), NDArrayFactory::create_<float>('c', {1000, 1000}));

    helper.runOperationSuit({&sb1, &sb2}, "ScalarAdd");
}



TEST_F(PlaygroundTests, Test_OpBenchmark_2) {

    BenchmarkHelper helper;
    Parameters parameters;
    parameters.addBoolParam("fOrder", true);
    float scalar = 2.0f;

    ScalarBenchmark sb(scalar::Multiply);

    // Y will be shared
    sb.setY(NDArrayFactory::create_<float>(scalar));

    auto generator = GENERATE_XZ() {
        // operands go together line by line
        x.push_back(NDArrayFactory::create_<float>('c', {100, 100}));
        z.push_back(NDArrayFactory::create_<float>('c', {100, 100}));

        x.push_back(NDArrayFactory::create_<float>('f', {1000, 1000}));
        z.push_back(NDArrayFactory::create_<float>('c', {1000, 1000}));

        // only share within single op call. do not cross share
        auto shared = NDArrayFactory::create_<float>('c', {256, 768});
        x.push_back(shared);
        z.push_back(shared);

        // using bool param here
        if (parameters.getBoolParam("fOrder")) {
            x.push_back(NDArrayFactory::create_<float>('c', {1000, 1000}));
            z.push_back(NDArrayFactory::create_<float>('f', {1000, 1000}));
        }

        //another way to call inplace op
        x.push_back(NDArrayFactory::create_<float>('c', {100, 100}));
        z.push_back(nullptr);
    };

    helper.runOperationSuit(&sb, generator, "ScalarTest");

    TransformBenchmark tb(transform::StrictOps::Tanh, "tanh");

    // we can use the same generator, since the same number of operands used
    helper.runOperationSuit(&tb, generator, "TransformTest");


    PairwiseBenchmark pb(pairwise::Pow, "pow test");

    auto generatorXYZ = GENERATE_XYZ() {
        x.push_back(NDArrayFactory::create_<float>('f', {100, 1000}));
        y.push_back(NDArrayFactory::create_<float>('c', {100, 1000}));
        z.push_back(NDArrayFactory::create_<float>('c', {100, 1000}));

        x.push_back(NDArrayFactory::create_<float>('f', {100, 1000}));
        y.push_back(NDArrayFactory::create_<float>('f', {100, 1000}));
        z.push_back(NDArrayFactory::create_<float>('f', {100, 1000}));
    };

    helper.runOperationSuit(&pb, generatorXYZ, "PairwiseTest");

    auto generatorReductionAxis = GENERATE_XYZ() {
        x.push_back(NDArrayFactory::create_<float>('c', {100, 1000}));

        // axis goes to y here
        y.push_back(NDArrayFactory::create_<int>('c', {1}, {(int) 0}));
        z.push_back(NDArrayFactory::create_<float>('c', {1000}));

        x.push_back(NDArrayFactory::create_<float>('c', {100, 1000}));
        y.push_back(NDArrayFactory::create_<int>('c', {1}, {1}));
        z.push_back(NDArrayFactory::create_<float>('c', {100}));

        // scalar case
        x.push_back(NDArrayFactory::create_<float>('c', {100, 1000}));
        y.push_back(nullptr);
        z.push_back(NDArrayFactory::create_<float>(0.0f));
    };


    ReductionBenchmark rb(reduce::FloatOps::Mean);

    helper.runOperationSuit(&rb, (const std::function<void (ResultSet &, ResultSet &, ResultSet &)>)(generatorReductionAxis), "ReductionAlongDimensionTest");
}

TEST_F(PlaygroundTests, Test_OpBenchmark_3) {

    TransformBenchmark tb(transform::StrictOps::Tanh, "tanh");
    PredefinedParameters a("alpha", {2, 3, 4});
    PredefinedParameters b("beta", {9, 15, 27});

    ParametersBatch batch({&a, &b});

    auto parameters = batch.parameters();
    ASSERT_EQ(9, parameters.size());

    auto params_0 = parameters[0];
    ASSERT_EQ(2, params_0.getIntParam("alpha"));
    ASSERT_EQ(9, params_0.getIntParam("beta"));

    auto params_1 = parameters[1];
    ASSERT_EQ(2, params_1.getIntParam("alpha"));
    ASSERT_EQ(15, params_1.getIntParam("beta"));

    auto params_3 = parameters[3];
    ASSERT_EQ(3, params_3.getIntParam("alpha"));
    ASSERT_EQ(9, params_3.getIntParam("beta"));
}

TEST_F(PlaygroundTests, Test_OpBenchmark_4) {

    BenchmarkHelper helper;

    PairwiseBenchmark pb(pairwise::Ops::Add, "PWT ADD");
    TransformBenchmark tb(transform::StrictOps::Tanh, "tanh");
    ScalarBenchmark sb(scalar::Multiply);
    sb.setY(NDArrayFactory::create_<float>(119.0f));

    PredefinedParameters a("alpha", {2, 3, 4});
    PredefinedParameters b("beta", {9, 15, 27});
    ParametersBatch batch({&a, &b});

    auto generator = PARAMETRIC_XZ() {
        // operands go together line by line
        x.push_back(NDArrayFactory::create_<float>('c', {p.getIntParam("alpha") , p.getIntParam("beta")}));
        z.push_back(NDArrayFactory::create_<float>('c', {p.getIntParam("alpha"), p.getIntParam("beta")}));
    };

    auto generatorXYZ = PARAMETRIC_XYZ() {
        // operands go together line by line
        x.push_back(NDArrayFactory::create_<float>('c', {p.getIntParam("alpha") , p.getIntParam("beta")}));
        y.push_back(NDArrayFactory::create_<float>('f', {p.getIntParam("alpha") , p.getIntParam("beta")}));
        z.push_back(NDArrayFactory::create_<float>('c', {p.getIntParam("alpha"), p.getIntParam("beta")}));
    };

    helper.runOperationSuit(&tb, generator, batch, "TransformTanh");
    helper.runOperationSuit(&sb, generator, batch, "ScalarMultiply");
    helper.runOperationSuit(&pb, generatorXYZ, batch, "PairwiseAdd");
}


TEST_F(PlaygroundTests, Test_OpBenchmark_5) {
    BenchmarkHelper helper;

    TransformBenchmark tb(transform::StrictOps::Sigmoid, "sigmoid");
    IntParameters length("length", 100, 500, 100);
    BoolParameters inplace("inplace");

    ParametersBatch batch({&length, &inplace});

    auto generator = PARAMETRIC_XZ() {
        // operands go together line by line
        auto arr = NDArrayFactory::create_<float>('c', {p.getIntParam("length")});
        x.push_back(arr);
        if(p.getIntParam("inplace") == 1){
            z.push_back(arr);
        } else {
            z.push_back(NDArrayFactory::create_<float>('c', {p.getIntParam("length")}));
        }
    };

    helper.runOperationSuit(&tb, generator, batch, "Transform_Sigmoid");
}

TEST_F(PlaygroundTests, Test_Something_5) {
    auto x = NDArrayFactory::create<float>('c', {100, 10});
    auto y = NDArrayFactory::create<float>('c', {10});
    auto z = NDArrayFactory::create<float>('c', {100, 10});
    std::vector<int> axis = {1};

    NativeOpExcutioner::execBroadcast(broadcast::Add, x.buffer(), x.shapeInfo(), y.buffer(), y.shapeInfo(), z.buffer(), z.shapeInfo(),
                                      axis.data(), axis.size(), nullptr, nullptr,
            nullptr, nullptr);
}

#define PARAMETRIC_D() [&] (Parameters &p) -> Context*
/*
TEST_F(PlaygroundTests, Test_OpBenchmark_6) {
    BenchmarkHelper helper;
    nd4j::ops::softmax op;
    DeclarableBenchmark db(op, "SoftMaxTest");

    PredefinedParameters a("alpha", {128, 256});
    PredefinedParameters b("beta", {1024, 2048});
    ParametersBatch batch({&a, &b});

    auto generator = PARAMETRIC_D() {
        auto ctx = new Context(1);

        ctx->setInputArray(0, NDArrayFactory::create_<float>('c', {p.getIntParam("alpha"), p.getIntParam("beta")}));
        ctx->setOutputArray(0, NDArrayFactory::create_<float>('c', {p.getIntParam("alpha"), p.getIntParam("beta")}));
        return ctx;
    };

    helper.runOperationSuit(&db, generator, batch, "parametrized softmax test");
}
*/

/*
TEST_F(PlaygroundTests, Test_Strided_Stuff) {
    auto array = NDArrayFactory::create<float>('c', {1048576, 1024});
    auto strided = array({0,0, 3, 4}, true);
    auto z = NDArrayFactory::create<float>(0.0f);
    //strided->shapeInfo()[shape::shapeInfoLength(strided->rankOf()) - 2] = 1024;

    int N = 1000;
    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < N; e++)
        NativeOpExcutioner::execReduceSameScalar(reduce::ReduceSameBenchmarkOp, strided.buffer(), strided.shapeInfo(), nullptr, z.buffer(), z.shapeInfo());

    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);

}
*/

/*
TEST_F(PlaygroundTests, StridedReductionsNoEWS) {
    nd4j_printf("SETTING ELEMENTWISE THRESHOLD AND TAD THRESHOLD TO 1/1","");
    nd4j::Environment::getInstance()->setElementwiseThreshold(1);
    nd4j::Environment::getInstance()->setTadThreshold(1);
    BenchmarkHelper helper;
    IntPowerParameters stride("stride", 2, 0, 10);          //2^0=1, ..., 2^10=1024
    ParametersBatch batch({&stride});
    //This is an edge case: technically an EWS *should* be available here
    auto generator1 = PARAMETRIC_XYZ() {
        auto stride = p.getIntParam("stride");
        auto arr = NDArrayFactory::create_<float>('c', {1048576 + (stride == 1 ? 0 : 1), stride});
        NDArray* strided;
        if(stride == 1){
            strided = arr;
        } else {
            strided = new NDArray((*arr)({0,1048576, 0,1}, true));        //All rows, first column
        }
        strided->assign(1.0);
        x.push_back(strided);
        y.push_back(nullptr);
        z.push_back(NDArrayFactory::create_<float>(0.0f));
    };
    ReductionBenchmark rbSum(reduce::SameOps::Sum, "stridedSum");
    helper.runOperationSuit(&rbSum, (const std::function<void (Parameters &, ResultSet &, ResultSet &, ResultSet &)>)(generator1), batch, "Strided Sum - No EWS Test 1");
    //No EWS defined for this case
    auto generator2 = PARAMETRIC_XYZ() {
        auto stride = p.getIntParam("stride");
        auto arr = NDArrayFactory::create_<float>('c', {(stride == 1 ? 1 : 2) * 1024, 1024, stride});
        NDArray* strided;
        if(stride == 1){
            strided = arr;
        } else {
            strided = new NDArray((*arr)({0,2*1024,2,  0,0,0,  0,1,1}, true, true));
        }
        strided->assign(1.0);
        x.push_back(strided);
        y.push_back(nullptr);
        z.push_back(NDArrayFactory::create_<float>(0.0f));
    };
    ReductionBenchmark rbSum2(reduce::SameOps::Sum, "stridedSumNoEWS");
    helper.runOperationSuit(&rbSum2, (const std::function<void (Parameters &, ResultSet &, ResultSet &, ResultSet &)>)(generator2), batch, "Strided Sum - No EWS Test 2");
}
*/

TEST_F(PlaygroundTests, LambdaTest_1) {
    auto array = NDArrayFactory::create<float>('c', {8192, 1024});
    array.linspace(1);

    auto lambda = LAMBDA_F(_x) {
        return _x + 32.12f;
    };

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < numIterations; e++) {
        array.applyLambda<float>(lambda);
    }


    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Lambda 1 time %lld us\n", outerTime / numIterations);
}




TEST_F(PlaygroundTests, LambdaTest_2) {
    auto array = NDArrayFactory::create<float>('c', {8192, 1024});
    auto row = NDArrayFactory::create<float>('c', {1, 1024});
    array.linspace(1);

    auto lambda = LAMBDA_F(_x) {
        return _x + 32.12f;
    };

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < numIterations; e++) {
        array.applyBroadcast(broadcast::Add, {1}, &row);
    }


    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Broadcast time %lld us\n", outerTime / numIterations);
}


TEST_F(PlaygroundTests, NoCacheTest_1) {
    std::vector<NDArray*> pool(poolSize);
    auto source = NDArrayFactory::create<float>('c', {8192, 1024});
    for (int e = 0; e < pool.size(); e++)
        pool[e] = source.dup();

    auto lambda = LAMBDA_F(_x) {
        return _x * 32.12f;
    };

    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    for (int e = 0; e < numIterations; e++) {
        auto v = pool[poolSize - 1 - (cnt++)];
        v->applyLambda<float>(lambda);

        if (cnt == poolSize)
            cnt = 0;
    }

    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Non-cached time %lld us\n", outerTime / numIterations);

    for (auto v: pool)
        delete v;
}


TEST_F(PlaygroundTests, NoCacheTest_2) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {8192, 1024});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }

    auto lambda = LAMBDA_FF(_x, _y) {
        return _x * 32.12f + _y;
    };

    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    for (int e = 0; e < numIterations; e++) {
        auto v1 = pool1[poolSize - 1 - cnt];
        auto v2 = pool2[cnt++];
        v1->applyPairwiseLambda<float>(v2, lambda);

        if (cnt == poolSize)
            cnt = 0;
    }

    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Non-cached PWT time %lld us\n", outerTime / numIterations);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}


TEST_F(PlaygroundTests, ReductionTest_1) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {1, 100});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }


    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    for (int e = 0; e < 100000; e++) {
        auto v = pool1[poolSize - 1 - cnt];
        auto r = v->sumNumber();

        if (cnt == poolSize)
            cnt = 0;
    }
    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::nanoseconds> (timeEnd - timeStart).count();
    auto outerTimeMs = std::chrono::duration_cast<std::chrono::milliseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Non-cached reduction time avg: %lld ns; Total time: %lld ms;\n", outerTime / 100000, outerTimeMs);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}


TEST_F(PlaygroundTests, ScalarTest_1) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {1, 100});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }


    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    float *buff = reinterpret_cast<float*>(source.buffer());
    for (int e = 0; e < 100000; e++) {
        //auto v = pool1[poolSize - 1 - cnt];
        //v->template applyScalar<simdOps::Add<float>>(2.0f);
        source.applyScalar(scalar::Add,2.0f);
        //functions::scalar::ScalarTransform<float>::template transformEx<simdOps::Add<float>>(source.buffer(), 1, source.buffer(), 1, 2.0f, nullptr, source.lengthOf());
        //functions::scalar::ScalarTransform<float>::template transform<simdOps::Add<float>>(buff, 1, buff, 1, 2.0f, nullptr, 100);

        cnt++;
        if (cnt == poolSize)
            cnt = 0;
    }
    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::nanoseconds> (timeEnd - timeStart).count();
    auto outerTimeMs = std::chrono::duration_cast<std::chrono::milliseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Cached scalar time avg: %lld ns; Total time: %lld ms;\n", outerTime / 100000L, outerTimeMs);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}


TEST_F(PlaygroundTests, ScalarTest_2) {
    std::vector<NDArray*> pool1(poolSize);
    std::vector<NDArray*> pool2(poolSize);
    auto source = NDArrayFactory::create<float>('c', {1, 100});
    for (int e = 0; e < pool1.size(); e++) {
        pool1[e] = source.dup();
        pool2[e] = source.dup();
    }


    auto timeStart = std::chrono::system_clock::now();
    int cnt = 0;
    float * array = reinterpret_cast<float*>(source.buffer());
    for (int e = 0; e < 100000; e++) {

#pragma omp simd
        for (int i = 0; i < source.lengthOf(); i++) {
            array[i] = simdOps::Add<float, float, float>::op(array[i], 2.0f);
        }

        cnt++;
        if (cnt == poolSize)
            cnt = 0;
    }
    auto timeEnd = std::chrono::system_clock::now();

    auto outerTime = std::chrono::duration_cast<std::chrono::nanoseconds> (timeEnd - timeStart).count();
    auto outerTimeMs = std::chrono::duration_cast<std::chrono::milliseconds> (timeEnd - timeStart).count();

    // nd4j_printf("Cached manual scalar time avg: %lld ns; Total time: %lld ms;\n", outerTime / 100000, outerTimeMs);

    for (auto v: pool1)
        delete v;

    for (auto v: pool2)
        delete v;
}

TEST_F(PlaygroundTests, Test_Profile_1) {
    GraphProfile prof;

    prof.setBuildTime(70);
    prof.setExecutionTime(130);

    prof.startEvent("omega");
    prof.spotEvent("alpha");
    prof.spotEvent("beta");
    prof.spotEvent("gamma");
    prof.recordEvent("omega");

    auto nodeA = prof.nodeById(1, "MatMul");
    auto nodeB = prof.nodeById(2, "Sum");
    auto nodeC = prof.nodeById(3, "Conv2D");

    nodeA->setObjectsSize(512);
    nodeA->setTemporarySize(65536);
    nodeA->setActivationsSize(512387);
    nodeA->setPreparationTime(127);
    nodeA->setExecutionTime(6539);


    nodeB->setObjectsSize(0);
    nodeB->setTemporarySize(0);
    nodeB->setActivationsSize(512387);
    nodeB->setPreparationTime(132);
    nodeB->setExecutionTime(2047);


    nodeC->setObjectsSize(1536);
    nodeC->setTemporarySize(2355674);
    nodeC->setActivationsSize(1022092);
    nodeC->setPreparationTime(129);
    nodeC->setExecutionTime(12983);

    // prof.printOut();
}

#ifdef GRAPH_FILES_OK
TEST_F(PlaygroundTests, Test_Profile_2) {
    Environment::getInstance()->setProfiling(true);
    auto graph = GraphExecutioner::importFromFlatBuffers("./resources/ae_00.fb");

    auto profile = GraphProfilingHelper::profile(graph, 2);
    // profile->printOut();

    delete graph;
    delete profile;
}
#endif

TEST_F(PlaygroundTests, Test_Im2Col_1) {
    
    int bS=16, iH=224,iW=224,  iC=3,oC=3,  kH=11,kW=11,  sH=4,sW=4,  pH=2,pW=2,  dH=1,dW=1;    
    int        oH=55, oW=55;
    int iterations = 1;

    auto input = NDArrayFactory::create<float>('c', {bS, iC, iH, iW});
    auto output = NDArrayFactory::create<float>('c', {bS, iC, kH, kW, oH, oW});

    auto outputPermuted = NDArrayFactory::create<float>('c', {bS, oH, oW, iC, kH, kW});
    outputPermuted.permutei({0, 3, 4, 5, 1, 2});

    nd4j::ops::im2col op;

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&output}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    // outputPermuted.printShapeInfo("permuted shape");

    auto permStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&outputPermuted}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto permEnd = std::chrono::system_clock::now();
    auto permTime = std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();


    auto legacyStart = std::chrono::system_clock::now();

    float extra[] = {(float)kH, (float)kW, (float)sH, (float)sW, (float)pH, (float)pW, (float)dH, (float)dW, 0.f, 0.f};
    for (int e = 0; e < iterations; e++) {
        input.applyTransform(transform::Im2col, &output, extra);
    }

    auto legacyEnd = std::chrono::system_clock::now();
    auto legacyTime = std::chrono::duration_cast<std::chrono::microseconds> (legacyEnd - legacyStart).count();


    auto legacyPermStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        input.applyTransform(transform::Im2col, &outputPermuted, extra);
    }

    auto legacyPermEnd = std::chrono::system_clock::now();
    auto legacyPermTime = std::chrono::duration_cast<std::chrono::microseconds> (legacyPermEnd - legacyPermStart).count();


    NativeOps nativeOps;

    Nd4jLong iArgs[] = {kH, kW, sH, sW, pH, pW, dH, dW, 0};
    Nd4jPointer inputBuffers[] = {input.buffer()};
    Nd4jPointer inputShapes[] = {input.shapeInfo()};

    Nd4jPointer outputBuffers[] = {output.buffer()};
    Nd4jPointer outputShapes[] = {output.shapeInfo()};

    auto javaStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        nativeOps.execCustomOp(nullptr, op.getOpHash(), inputBuffers, inputShapes, 1, outputBuffers, outputShapes, 1, nullptr, 0, iArgs, 9, nullptr, 0, false);
    }

    auto javaEnd = std::chrono::system_clock::now();
    auto javaTime = std::chrono::duration_cast<std::chrono::microseconds> (javaEnd - javaStart).count();


    Nd4jPointer outputPermBuffers[] = {outputPermuted.buffer()};
    Nd4jPointer outputPermShapes[] = {outputPermuted.shapeInfo()};

    auto javaPermStart = std::chrono::system_clock::now();


    for (int e = 0; e < iterations; e++) {
        nativeOps.execCustomOp(nullptr, op.getOpHash(), inputBuffers, inputShapes, 1, outputPermBuffers, outputPermShapes, 1, nullptr, 0, iArgs, 9, nullptr, 0, false);
    }

    auto javaPermEnd = std::chrono::system_clock::now();
    auto javaPermTime = std::chrono::duration_cast<std::chrono::microseconds> (javaPermEnd - javaPermStart).count();

    // nd4j_printf("New time: %lld us;\n", outerTime / iterations);
    // nd4j_printf("Permuted time: %lld us;\n", permTime / iterations);
    // nd4j_printf("Legacy time: %lld us;\n", legacyTime / iterations);
    // nd4j_printf("Legacy Permuted time: %lld us;\n", legacyPermTime / iterations);
    // nd4j_printf("Java time: %lld us;\n", javaTime / iterations);
    // nd4j_printf("Java Permuted time: %lld us;\n", javaPermTime / iterations);
}

TEST_F(PlaygroundTests, Test_Im2Col_2) {
    auto input = NDArrayFactory::create<float>('c', {16, 3, 224, 224});
    auto output = NDArrayFactory::create<float>('c', {16, 3, 11, 11, 55, 55});

    auto outputPermuted = NDArrayFactory::create<float>('c', {16, 55, 55, 3, 11, 11});
    outputPermuted.permutei({0, 3, 4, 5, 1, 2});

    nd4j::ops::im2col op;

    Nd4jLong iArgs[] = {11, 11, 4, 4, 2, 2, 1, 1, 0};
    Nd4jPointer inputBuffers[] = {input.buffer()};
    Nd4jPointer inputShapes[] = {input.shapeInfo()};

    Nd4jPointer outputPermBuffers[] = {outputPermuted.buffer()};
    Nd4jPointer outputPermShapes[] = {outputPermuted.shapeInfo()};

    NativeOps nativeOps;

    nativeOps.execCustomOp(nullptr, op.getOpHash(), inputBuffers, inputShapes, 1, outputPermBuffers, outputPermShapes, 1, nullptr, 0, iArgs, 9, nullptr, 0, false);
}

TEST_F(PlaygroundTests, Test_Col2Im_1) {
    
    int bS=16, iH=224,iW=224,  iC=3,oC=3,  kH=11,kW=11,  sH=4,sW=4,  pH=2,pW=2,  dH=1,dW=1;    
    int        oH=55, oW=55;
    int iterations = 1;

    auto input = NDArrayFactory::create<float>('c', {bS, iC, kH, kW, oH, oW});
    auto output = NDArrayFactory::create<float>('c', {bS, iC, iH, iW});
    
    auto inputPermuted = NDArrayFactory::create<float>('c', {bS, oH, oW, iC, kH, kW});
    inputPermuted.permutei({0, 3, 4, 5, 1, 2});
    auto outputPermuted = NDArrayFactory::create<float>('c', {bS, iH, iW, iC});
    outputPermuted.permutei({0, 3, 1, 2});

    input = 10.;
    output = 2.;

    inputPermuted = 10.;
    outputPermuted = 2.;

    nd4j::ops::col2im op;

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&output}, {}, {sH, sW, pH, pW, iH, iW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    auto permStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&inputPermuted}, {&outputPermuted}, {}, {sH, sW, pH, pW, iH, iW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto permEnd = std::chrono::system_clock::now();
    auto permTime = std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();

    // nd4j_printf("C-order  time: %lld us;\n", outerTime / iterations);
    // nd4j_printf("Permuted time: %lld us;\n", permTime / iterations);    
}

TEST_F(PlaygroundTests, Test_Im2Col_3) {
    
    int bS=16, iH=224,iW=224,  iC=3,oC=3,  kH=11,kW=11,  sH=4,sW=4,  pH=2,pW=2,  dH=1,dW=1;    
    int        oH=55, oW=55;
    int iterations = 1;

    auto output = NDArrayFactory::create<float>('c', {bS, iC, kH, kW, oH, oW});
    auto input = NDArrayFactory::create<float>('c', {bS, iC, iH, iW});
    
    auto outputPermuted = NDArrayFactory::create<float>('c', {bS, oH, oW, iC, kH, kW});
    outputPermuted.permutei({0, 3, 4, 5, 1, 2});
    auto inputPermuted = NDArrayFactory::create<float>('c', {bS, iH, iW, iC});
    inputPermuted.permutei({0, 3, 1, 2});

    input = 10.;
    output = 2.;

    inputPermuted = 10.;
    outputPermuted = 2.;

    nd4j::ops::im2col op;

    auto timeStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&input}, {&output}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();

    auto permStart = std::chrono::system_clock::now();

    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&inputPermuted}, {&outputPermuted}, {}, {kH, kW, sH, sW, pH, pW, dH, dW, 0}, {});
        ASSERT_EQ(Status::OK(), result);
    }

    auto permEnd = std::chrono::system_clock::now();
    auto permTime = std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();

    // nd4j_printf("C-order  time: %lld us;\n", outerTime / iterations);
    // nd4j_printf("Permuted time: %lld us;\n", permTime / iterations);    
}


TEST_F(PlaygroundTests, loop_test_1) {

    auto f = NDArrayFactory::create<float>('c', {2}, {5000, 10000});
    nd4j::ops::randomuniform op;

    auto result = op.execute({&f}, {-1.0f, 1.0f}, {});
    ASSERT_EQ(Status::OK(), result->status());

    auto array = result->at(0);

    auto buffer = array->buffer();
    int cnt = 0;
    int iterations = 1;

    //nd4j_printf("Array length: %lld\n", array->lengthOf());

    int length = (int) array->lengthOf();
    int span = (int) (array->lengthOf() / 6) + 8;

    NativeOps ops;

    auto t = new int[1000000];




    FloatBits fb;
    float threshold = 0.99f;
    fb.f_ = threshold;
    int le = ops.estimateThreshold(nullptr, reinterpret_cast<void *>(array->buffer()), array->shapeInfo(), static_cast<int>(array->lengthOf()), threshold);

    t[0] = le;
    t[1] = length;
    t[2] = fb.i_;

    //nd4j_printf("number of elements: [%i]\n", le);

    long permTime = 0;

    for (int x = 0; x < iterations; x++) {
        auto permStart = std::chrono::system_clock::now();
        ops.estimateThreshold(nullptr, reinterpret_cast<void *>(array->buffer()), array->shapeInfo(), static_cast<int>(array->lengthOf()), threshold);
        TypeCast::convertToThreshold<float>(nullptr, buffer, array->lengthOf(), t);

        auto permEnd = std::chrono::system_clock::now();
        permTime += std::chrono::duration_cast<std::chrono::microseconds> (permEnd - permStart).count();
    }



    nd4j_printf("Permuted time: %lld us; Counter: %i;\n", permTime / iterations, cnt);

    delete result;
    delete[] t;
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, ndarray_tile_test1) {

    auto x = NDArrayFactory::create<float>('c', {20, 30});
    auto exp = NDArrayFactory::create<float>('c', {2,40,60});

    auto timeStart = std::chrono::system_clock::now();
    auto tiled = x.tile({2,2,2});
    auto timeEnd = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
    // nd4j_printf("c-order time: %d;\n", time);
    
    ASSERT_TRUE(tiled.isSameShape(&exp)); 
}


//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, ndarray_tile_test2) {

    auto x = NDArrayFactory::create<float>('f', {20, 30});
    auto exp = NDArrayFactory::create<float>('f', {2,40,60});

    auto timeStart = std::chrono::system_clock::now();
    auto tiled = x.tile({2,2,2});
    auto timeEnd = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
    // nd4j_printf("f-order time: %d;\n", time);
    
    ASSERT_TRUE(tiled.isSameShape(&exp)); 
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loopThroughArrs_test1) {
    
    NDArray x('c', {20, 30, 40}, nd4j::DataType::DOUBLE);
    NDArray y('f', {50, 30, 4, 4}, nd4j::DataType::DOUBLE);  

    auto xBuff = x.bufferAsT<double>();
    auto yBuff = y.bufferAsT<double>();

    auto len = x.lengthOf();

    //***********************************
    //***********************************
  
    auto timeStart = std::chrono::system_clock::now();
#pragma omp parallel for schedule(guided) 
    for(Nd4jLong i = 0; i < len; ++i) {
                
        Nd4jLong offset1 = shape::getIndexOffset(i, x.getShapeInfo(), len);
        Nd4jLong offset2 = shape::getIndexOffset(i, y.getShapeInfo(), len);
        
        xBuff[offset1] = yBuff[offset2];
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto myTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
    
    //***********************************
    //***********************************
    
    timeStart = std::chrono::system_clock::now();
#pragma omp parallel for schedule(guided)
    for(Nd4jLong i = 0; i < len; ++i) {
        
        Nd4jLong offset1 = shape::getIndexOffset(i, x.getShapeInfo(), len);
        Nd4jLong offset2 = shape::getIndexOffset(i, y.getShapeInfo(), len);
        xBuff[offset1] = yBuff[offset2];
    }
    timeEnd = std::chrono::system_clock::now();
    auto oldTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();
   
    nd4j_printf("My  time: %lld us;\n", myTime);
    nd4j_printf("Old time: %lld us;\n", oldTime);

    ASSERT_TRUE(1);
}


static void loopSpan(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
            
    BlockInformation info(len, ELEMENT_THRESHOLD);
    #pragma omp parallel num_threads(info.threads) if (info.threads > 1) default(shared)
    {                
        auto i = omp_get_thread_num();            
        Nd4jLong itemsToLoop = (i < info.threads-1) ? info.items : info.items + info.remainder;
        Nd4jLong index = i * info.items;
        auto xi = x + xEws * index;
        auto yi = y + yEws * index;
        auto zi = z + zEws * index;        
        #pragma omp simd
        for (Nd4jLong j = 0; j < itemsToLoop; j++) 
            zi[j * zEws] = simdOps::LogPoissonLoss<float, float, float>::op(xi[j * xEws], yi[j * yEws]);
    }
}

static void loopSimple(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
    int threads = 6;
    int span_size = len / threads + 1;
    
    #pragma omp parallel for simd schedule(static, span_size) if (len > ELEMENT_THRESHOLD) proc_bind(close) default(shared)
    for(Nd4jLong i = 0; i < len; ++i)
        z[i * zEws] = simdOps::LogPoissonLoss<float, float, float>::op(x[i * xEws], y[i * yEws]);

}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loopThroughArrs_test2) {
    
    NDArray x('c', {40, 25}, nd4j::DataType::FLOAT32);

    const int iterations = 10;
    const int arrays = 10;

    std::vector<NDArray> arrs(arrays);
    for(auto& arr : arrs)
        arr = x;
    
    //***********************************    
    auto timeStart = std::chrono::system_clock::now();
    srand(119);
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        
    
        loopSimple(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto simpleTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    //***********************************
    timeStart = std::chrono::system_clock::now();
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        

        loopSpan(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);    
    }
    timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    nd4j_printf("simple time: %lld us;\n", simpleTime);
    nd4j_printf("span   time: %lld us;\n", spanTime);

    ASSERT_TRUE(1);        
}

static void loop1(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
            
    nd4j::OmpLaunchHelper info(len);
    #pragma omp parallel num_threads(info._numThreads) default(shared)
    {                
        auto threadNum = omp_get_thread_num();
        Nd4jLong threadOffset = info.getThreadOffset(threadNum);        
        #pragma omp simd
        for (Nd4jLong j = 0; j < info.getItersPerThread(threadNum); j++)  {
            Nd4jLong xOffset = shape::getIndexOffset(j+threadOffset, xShapeInfo, len);
            Nd4jLong yOffset = shape::getIndexOffset(j+threadOffset, yShapeInfo, len);
            Nd4jLong zOffset = shape::getIndexOffset(j+threadOffset, zShapeInfo, len);
            z[xOffset] = simdOps::LogPoissonLoss<float, float, float>::op(x[xOffset], y[xOffset]);
        }
    }
}

static void loop2(float* x, Nd4jLong* xShapeInfo, float* y, Nd4jLong* yShapeInfo, float* z, Nd4jLong* zShapeInfo) {

    auto len = shape::length(xShapeInfo);
    int xEws = shape::elementWiseStride(xShapeInfo);
    int yEws = shape::elementWiseStride(yShapeInfo);
    int zEws = shape::elementWiseStride(zShapeInfo);
    int threads = 6;
    int span_size = len / threads + 1;
    
    #pragma omp parallel for simd schedule(static) default(shared)
    for(Nd4jLong i = 0; i < len; ++i) {
        Nd4jLong xOffset = shape::getIndexOffset(i, xShapeInfo, len);
        Nd4jLong yOffset = shape::getIndexOffset(i, yShapeInfo, len);
        Nd4jLong zOffset = shape::getIndexOffset(i, zShapeInfo, len);
        z[xOffset] = simdOps::LogPoissonLoss<float, float, float>::op(x[xOffset], y[xOffset]);
    }

}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loopThroughArrs_test3) {
    
    NDArray x('c', {50, 250}, nd4j::DataType::FLOAT32);

    const int iterations = 100;
    const int arrays = 100;

    std::vector<NDArray> arrs(arrays);
    for(auto& arr : arrs)
        arr = x;
    
    //***********************************    
    auto timeStart = std::chrono::system_clock::now();
    srand(119);
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        
    
        loop2(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto simpleTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    //***********************************
    timeStart = std::chrono::system_clock::now();
    for(Nd4jLong i = 0; i < iterations; ++i) {
        int xInd = rand() % arrays;
        int yInd = rand() % arrays;
        int zInd = rand() % arrays;
        auto xBuff = arrs[xInd].bufferAsT<float>();
        auto yBuff = arrs[yInd].bufferAsT<float>();
        auto zBuff = arrs[zInd].bufferAsT<float>();
        auto xShapeInfo = arrs[xInd].getShapeInfo();
        auto yShapeInfo = arrs[yInd].getShapeInfo();
        auto zShapeInfo = arrs[zInd].getShapeInfo();        

        loop1(xBuff, xShapeInfo, yBuff, yShapeInfo, zBuff, zShapeInfo);    
    }
    timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();

    nd4j_printf("simpleTime time: %lld us;\n", simpleTime);
    nd4j_printf("spanTime   time: %lld us;\n", spanTime);

    ASSERT_TRUE(1);        
}

TEST_F(PlaygroundTests, test_batched_skipgram_1) {
    const int batchSize = 64;
    const int codeLen = 6;
    const int numWords = 244;
    const int vectorLength = 50;

    auto target = NDArrayFactory::create<int>('c', {batchSize});
    auto ngStarter = NDArrayFactory::empty<int>();
    auto indices = NDArrayFactory::create<int>('c', {batchSize, codeLen});
    auto codes = NDArrayFactory::create<int8_t>('c', {batchSize, codeLen});
    auto syn0 = NDArrayFactory::create<float>('c', {numWords, vectorLength});
    auto syn1 = NDArrayFactory::create<float>('c', {numWords, vectorLength});
    auto syn1Neg = NDArrayFactory::empty<float>();
    auto expTable = NDArrayFactory::linspace<float>(0.001, 0.995, 10000);
    auto negTable = NDArrayFactory::empty<float>();

    auto alpha = NDArrayFactory::create<double>('c', {batchSize});
    auto randomValue = NDArrayFactory::create<Nd4jLong>('c', {batchSize});
    auto inferenceVector = NDArrayFactory::empty<float>();

    syn0.assign(0.01);
    syn1.assign(0.02);

    Nd4jLong rv = 2843242345121L;
    auto lr = 0.025;
    for (int e = 0; e < batchSize; e++) {
        target.p(e, e);
        alpha.p(e, lr);
        randomValue.p(e, rv);

        lr -= 0.001;


        for (int s = 1; s < codeLen; s++) {
            indices.p(e, s, nd4j::math::nd4j_abs<Nd4jLong>(rv % numWords));
            codes.p(e, s, s % 2);

            rv = nd4j::math::nd4j_abs<Nd4jLong>(rv * 25214903917L + 11);
        }

        rv = nd4j::math::nd4j_abs<Nd4jLong>(rv * 25214903917L + 11);
    }

    //indices.printIndexedBuffer("indices");
    //codes.printIndexedBuffer("codes");

    auto iterations = 10;

    nd4j::ops::skipgram op;

    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        auto result = op.execute({&target, &ngStarter, &indices, &codes, &syn0, &syn1, &syn1Neg, expTable, &negTable, &alpha, &randomValue, &inferenceVector}, {}, {}, {false}, true);
        ASSERT_EQ(Status::OK(), result->status());
        delete result;
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();


    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);


    delete expTable;
}


TEST_F(PlaygroundTests, test_reduce_scalar_float_1) {
    auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
    auto target = NDArrayFactory::create<float>(0.0f);

    // warm up
    for (int e = 0; e < 5; e++) {
        NativeOpExcutioner::execReduceFloatScalar(reduce::Mean, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
    }

    int iterations = 10;
    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        NativeOpExcutioner::execReduceFloatScalar(reduce::Mean, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);
}

TEST_F(PlaygroundTests, test_reduce_scalar_float_2) {
    auto array = NDArrayFactory::create<float>('c', {100000});
    auto target = NDArrayFactory::create<float>(0.0f);

     // warm up
     for (int e = 0; e < 100; e++) {
         NativeOpExcutioner::execReduceFloatScalar(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
     }

     int iterations = 100;
     auto timeStart = std::chrono::system_clock::now();
     for (int e = 0; e < iterations; e++) {
         NativeOpExcutioner::execReduceFloatScalar(reduce::ReduceFloatBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
     }
     auto timeEnd = std::chrono::system_clock::now();
     auto spanTime = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)/iterations).count();
     auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

     nd4j_printf("average time: %lld ns;\n", spanTime);
     nd4j_printf("total time: %lld ms;\n", ttlTime);
}

TEST_F(PlaygroundTests, test_reduce_scalar_same_2) {
    auto array = NDArrayFactory::create<float>('c', {100000});
    auto target = NDArrayFactory::create<float>(0.0f);

    // warm up
    for (int e = 0; e < 100; e++) {
        NativeOpExcutioner::execReduceSameScalar(reduce::ReduceSameBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
    }

    int iterations = 100;
    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        NativeOpExcutioner::execReduceSameScalar(reduce::ReduceSameBenchmarkOp, array.buffer(), array.shapeInfo(), nullptr, target.buffer(), target.shapeInfo());
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::nanoseconds> ((timeEnd - timeStart)/iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("average time: %lld ns;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);
}


TEST_F(PlaygroundTests, test_assign_float) {
     // auto array = NDArrayFactory::create<float>('c', {32, 128, 256, 256});
     // auto target = NDArrayFactory::create<float>('c', {32, 128, 256, 256});

     auto array = NDArrayFactory::create<float>('c', {32, 64, 128, 128});
     auto target = NDArrayFactory::create<float>('c', {32, 64, 128, 128});

     array.assign(119);

     // warm up
     for (int e = 0; e < 5; e++) {
         NativeOpExcutioner::execTransformAny(transform::Assign, array.buffer(), array.shapeInfo(), target.buffer(), target.shapeInfo(), nullptr, nullptr, nullptr);
     }

     int iterations = 10;
     auto timeStart = std::chrono::system_clock::now();
     for (int e = 0; e < iterations; e++) {
         NativeOpExcutioner::execTransformAny(transform::Assign, array.buffer(), array.shapeInfo(), target.buffer(), target.shapeInfo(), nullptr, nullptr, nullptr);
     }
     auto timeEnd = std::chrono::system_clock::now();
     auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart)/iterations).count();
     auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();
     auto bw = (1000000L * (float) (array.lengthOf() * array.sizeOfT()) / spanTime) / 1024 / 1024 / 1024;

     nd4j_printf("average time: %lld us;\n", spanTime);
     nd4j_printf("total time: %lld ms;\n", ttlTime);
     nd4j_printf("Bandwidth: %f GB/s\n", bw)

}

/*
TEST_F(PlaygroundTests, test_manual_loop) {
    const unsigned int len = 32 * 128 * 256 * 256;
    auto array = new float[len];
    auto z = new float[len];

    for (unsigned int e = 0; e < len; e++)
        array[e] = (float) e;

    const int iterations = 100;

    auto timeStart = std::chrono::system_clock::now();
    for (int i = 0; i < iterations; i++) {

#pragma omp parallel for num_threads(4) schedule(static, 32768)
        for (unsigned int e = 0; e < len; e++)
            z[e] = array[e];
    }

    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();
    auto bw = (1000000L * (float) (len * sizeof(float)) / spanTime) / 1024 / 1024 / 1024;

    nd4j_printf("length: %i\n", len);
    nd4j_printf("average time: %lld us;\n", spanTime);
    nd4j_printf("total time: %lld ms;\n", ttlTime);
    nd4j_printf("Bandwidth: %f GB/s\n", bw)

    delete[] array;
    delete[] z;
}
*/

TEST_F(PlaygroundTests, test_col2im_permuted_1) {
    auto x = NDArrayFactory::create<float>('c', {8, 64, 55, 55, 3, 3});
    x.assign(1.f);
    x.permutei({0, 1, 4, 5, 2, 3});

    auto z0 = NDArrayFactory::create<float>('c', {64, 8, 112, 112});
    z0.permutei({1, 0, 2, 3});

    auto z1 = NDArrayFactory::create<float>('c', {64, 8, 112, 112});
    z1.permutei({1, 0, 2, 3});

    nd4j_printf("Starting custom run...\n","");
    const int iterations = 100;
    nd4j::ops::col2im op;

    auto timeStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        op.execute({&x}, {&z0}, {}, {2, 2, 0, 0, 112, 112, 1, 1, 1}, {});
    }
    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();

    nd4j_printf("Starting legacy run...\n","");
    std::array<float, 8> extra = {2.f, 2.f, 0.f, 0.f, 112.f, 112.f, 1.f, 1.f};

    auto legacyStart = std::chrono::system_clock::now();
    for (int e = 0; e < iterations; e++) {
        x.applyTransform(transform::Col2Im, &z1, extra.data());
    }
    auto legacyEnd = std::chrono::system_clock::now();
    auto legacySpanTime = std::chrono::duration_cast<std::chrono::microseconds> ((legacyEnd - legacyStart) / iterations).count();
    auto legacyTtlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((legacyEnd - legacyStart)).count();

    nd4j_printf("average time: %lld us vs %lld us;\n", spanTime, legacySpanTime);
    nd4j_printf("total time: %lld ms vs %lld ms;\n", ttlTime, legacyTtlTime);

    ASSERT_EQ(z0, z1);
}


TEST_F(PlaygroundTests, test_addi_assign) {
    int iterations = 1;
    auto x = NDArrayFactory::create<float>('c', {1000000000});
    auto z = NDArrayFactory::create<float>('c', {1000000000});
    x.assign(119.0f);

    auto timeStart = std::chrono::system_clock::now();

    x.applyScalar(scalar::Add,1.0f, &z, nullptr);
    //z.assign(x);

    auto timeEnd = std::chrono::system_clock::now();
    auto spanTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / iterations).count();
    auto ttlTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart)).count();
    auto bw = (1000000L * (float) (x.lengthOf() * x.sizeOfT()) / spanTime) / 1024 / 1024 / 1024;

    nd4j_printf("Avg add(1.0f) time: %lld us\n", spanTime);
    nd4j_printf("Bandwidth: %f GB/s\n", bw);
}

/*
/////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, conv2d_1) {

    const int N = 100;
    int bS=8, iH=64,iW=64,  iC=32,oC=32,  kH=2,kW=2,  sH=1,sW=1,  pH=0,pW=0,  dH=1,dW=1;
    int paddingMode = 1;             // 1-SAME, 0-VALID;
    int dataFormat  = 0;             // 1-NHWC, 0-NCHW

    NDArray input('c', {bS, iC, iH, iW}, nd4j::DataType::FLOAT32);
    NDArray output(input);
    NDArray weights('c', {kH, kW, iC, oC}, nd4j::DataType::FLOAT32);
    NDArray bias('c', {oC}, nd4j::DataType::FLOAT32);
    input = 2.;
    weights.linspace(0.1, 0.1);
    bias = 0.5;

    nd4j::ops::conv2d op;
    for (int i = 0; i < 10; i++)
    	100.5*0.5;

    auto timeStart = std::chrono::system_clock::now();

    for (int i = 0; i < N; i++)
        op.execute({&input, &weights, &bias}, {&output} , {}, {kH,kW,  sH,sW,  pH,pW,  dH,dW, paddingMode, dataFormat},{});

    auto timeEnd = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();
    printf("duration %ld\n", duration);
}
*/

/////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, batchnorm_1) {

    const int N   = 100;
    NDArray input   ('c', {8, 32, 64, 64}, nd4j::DataType::FLOAT32);
    NDArray output  ('c', {8, 32, 64, 64}, nd4j::DataType::FLOAT32);
    NDArray mean    ('c', {32}, nd4j::DataType::FLOAT32);
    NDArray variance('c', {32}, nd4j::DataType::FLOAT32);
    NDArray gamma   ('c', {32}, nd4j::DataType::FLOAT32);
    NDArray beta    ('c', {32}, nd4j::DataType::FLOAT32);

    input = 10.5;
    mean = 5.5;
    variance = 1.5;
    gamma = 0.5;
    beta = 2.5;

    nd4j::ops::batchnorm_new op;

    auto timeStart = std::chrono::system_clock::now();
    // for (int i = 0; i <N ; i++)
        op.execute({&input, &mean, &variance, &gamma, &beta}, {&output}, {1e-5}, {1,1,1}, {});
    auto timeEnd = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();

    printf("duration %ld\n", duration);
}




//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, softmax_1) {
    
    const int N = 100;
    NDArray input('c', {1024, 256}, nd4j::DataType::FLOAT32);
    NDArray output('c', {1024, 256}, nd4j::DataType::FLOAT32);

    input.linspace(-100., 0.01);

    nd4j::ops::softmax op;

    for (int i = 0; i < 20 ; i++)
    	100.5*100.5;

    auto timeStart = std::chrono::system_clock::now();

	for (int i = 0; i < N ; i++)
        op.execute({&input}, {&output}, {}, {1}, {});

    auto timeEnd = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();
    printf("duration %ld\n", duration);

}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, subarr_1) {

    NDArray x('c', {10, 5}, nd4j::DataType::FLOAT32);
    NDArray subArr1 = x({0,0,  3,4});
    NDArray subArr2 = x({0,0,  3,4}, true);

    subArr1.printShapeInfo("subArr1");
    subArr2.printShapeInfo("subArr2");
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, subarr_2) {

    NDArray x('c', {10, 5}, nd4j::DataType::FLOAT32);
    auto subArr1 = x.subarray({NDIndex::all(), NDIndex::point(2)});

    subArr1->printShapeInfo("subArr1");

    ASSERT_EQ(5, subArr1->ews());
    delete subArr1;
}

////////////////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loops_1) {
/*
    const int N = 1;
    NDArray x('c', {16, 32, 64, 64}, nd4j::DataType::FLOAT32);
    NDArray z1('c', {32}, nd4j::DataType::FLOAT32);
    NDArray z2('c', {32}, nd4j::DataType::FLOAT32);
    NDArray z3('c', {32}, nd4j::DataType::FLOAT32);
    std::vector<int> dimsToExclude = {0,2,3};
    std::vector<int> tadDims = {1};
    x.linspace(0.01);

    // warm up
    for (int i = 0; i < 1000; ++i)
        32*512;

    auto timeStart1 = std::chrono::system_clock::now();
    for (int i = 0; i < N ; i++)
        x.reduceAlongDimension(nd4j::reduce::Mean, &z1, dimsToExclude);
    auto timeEnd1  = std::chrono::system_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd1 - timeStart1) / N).count();


    auto timeStartE = std::chrono::system_clock::now();
    for (int i = 0; i < N ; i++)
        x.reduceAlongDimension(nd4j::reduce::Sum, &z3, dimsToExclude);
    auto timeEndE  = std::chrono::system_clock::now();
    auto durationE = std::chrono::duration_cast<std::chrono::microseconds> ((timeEndE - timeStartE) / N).count();

    Nd4jLong *tadShapeInfo(nullptr), *tadOffsets(nullptr);
    x.getSubArrShapeAndOffsets(tadDims, tadShapeInfo, tadOffsets);

    // shape::printShapeInfoLinear(tadShapeInfo);
    // shape::printIntArray(tadOffsets, 32);

    auto timeStart2 = std::chrono::system_clock::now();

    for (int i = 0; i < N ; i++)
        Loops::loopReduce<float, float, float>(x.bufferAsT<float>(), tadShapeInfo, tadOffsets,
                                       z2.bufferAsT<float>(), z2.getShapeInfo(),
                                       nullptr,
                                       &simdOps::Mean<float,float>::startingValue,
                                       &simdOps::Mean<float,float>::update,
                                       &simdOps::Mean<float,float>::op,
                                       &simdOps::Mean<float,float>::postProcess);


    auto timeEnd2  = std::chrono::system_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd2 - timeStart2) / N).count();

    RELEASE(tadShapeInfo, x.getWorkspace());
    RELEASE(tadOffsets, x.getWorkspace());

    // z1.printIndexedBuffer("z1 ");
    // z2.printIndexedBuffer("z2 ");

    ASSERT_TRUE(z1.equalsTo(z2));

    printf("duration old: %ld\n", duration1);
    printf("duration new: %ld\n", duration2);
    printf("duration E: %ld\n", durationE);
*/
}

////////////////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, newTads_1) {

    const int N = 1000;
    
    Nd4jLong shapeInfo[] = {4, 1024,1024,1024,1024,  1024*1024*1024,1024*1024,1024,1,  16384,1,99};
    const int rank = shape::rank(shapeInfo);
    const std::vector<int> dimsToExclude = {1,3};
    const std::vector<int> tadDims       = {0,2};
    const bool keepUnitesInShape = false;

    const Nd4jLong numOfSubArrs = ShapeUtils::getNumOfSubArrs(shapeInfo, dimsToExclude);
    const int subArrRank = (rank == dimsToExclude.size() || keepUnitesInShape) ? rank : rank - dimsToExclude.size();

    auto sPtr = new Nd4jLong[shape::shapeInfoLength(subArrRank)];
    auto oPtr = new Nd4jLong[numOfSubArrs];

    // warm up
    for (int i = 0; i < 1000; ++i)
        32*512;

    auto timeStart = std::chrono::system_clock::now();
    for (int i = 0; i < N ; i++)
        auto tadPack = nd4j::ConstantTadHelper::getInstance()->tadForDimensions(shapeInfo, tadDims);  
    auto timeEnd  = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();

    printf("duration old: %ld\n", duration);
    delete []sPtr;
    delete []oPtr;
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loops_2) {
    
    const uint N = 5;
    const Nd4jLong dim0(10), dim1(10), dim2(10);

    const Nd4jLong shapeInfo[2*3+4] = {3, dim0,dim1,dim2,  1,dim0,dim0*dim1,  8192,1,102};
    const Nd4jLong len = shape::length(shapeInfo);
    float* buff = new float[len];
    
    const Nd4jLong* shape   = shape::shapeOf(const_cast<Nd4jLong*>(shapeInfo));
    const Nd4jLong* strides = shape::stride(const_cast<Nd4jLong*>(shapeInfo));

    // OmpLaunchHelper threadsInfo(len);

    Nd4jLong *xOffsets, *yOffsets, *zOffsets;
    xOffsets = new Nd4jLong[len];
    yOffsets = new Nd4jLong[len];
    zOffsets = new Nd4jLong[len];
    

    // warm up
    for (int i = 0; i < 1000; ++i) 32*512;


    //***********************************
    //***********************************  
    auto timeStart = std::chrono::system_clock::now();

    for (int i = 0; i < N; ++i)
    {
            
        #pragma omp parallel sections
        {
            #pragma omp section
            {
    
                shape::calcOffsets(3, shape, strides, xOffsets);            
            } 
            #pragma omp section
            {
                
                shape::calcOffsets(3, shape, strides, yOffsets);
            } 
            #pragma omp section
            {
                
                shape::calcOffsets(3, shape, strides, zOffsets);
            }    
        }
     
    
        PRAGMA_OMP_PARALLEL_FOR_SIMD
        for (uint i = 0; i < len; i++) 
            buff[zOffsets[i]] = buff[xOffsets[i]] * buff[yOffsets[i]];
    } 
    auto timeEnd = std::chrono::system_clock::now();
    auto myTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N) .count();
    
    //***********************************
    //***********************************
    
    timeStart = std::chrono::system_clock::now();    

    uint xShapeInfoCast[MAX_RANK];
    uint yShapeInfoCast[MAX_RANK];
    uint zShapeInfoCast[MAX_RANK];

    bool canCastX = DataTypeUtils::castShapeInfo(shapeInfo, xShapeInfoCast);
    bool canCastY = DataTypeUtils::castShapeInfo(shapeInfo, yShapeInfoCast);
    bool canCastZ = DataTypeUtils::castShapeInfo(shapeInfo, zShapeInfoCast);

    for (int i = 0; i < N; ++i)
    {
        PRAGMA_OMP_PARALLEL_FOR_SIMD
        for (uint i = 0; i < len; i++) {
            
            auto xOffset = shape::indexOffset(i, shapeInfo, xShapeInfoCast, len, canCastX);
            auto yOffset = shape::indexOffset(i, shapeInfo, yShapeInfoCast, len, canCastY);
            auto zOffset = shape::indexOffset(i, shapeInfo, zShapeInfoCast, len, canCastZ);
            buff[zOffset] = buff[xOffset] * buff[yOffset];
        }
                 
    // PRAGMA_OMP_PARALLEL_FOR_SIMD_COLLAPSE(1)
    // for (uint i0 = 0; i0 < shape[0]; ++i0)
    //     for (uint i1 = 0; i1 < shape[1]; ++i1)
    //         for (uint i2 = 0; i2 < shape[2]; ++i2)
    //             buff[i0*strides[0]+i1*strides[1]+i2*strides[2]] = buff[i0*strides[0]+i1*strides[1]+i2*strides[2]] * buff[i0*strides[0]+i1*strides[1]+i2*strides[2]];
    }
    timeEnd = std::chrono::system_clock::now();
    auto oldTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();
   
    nd4j_printf("My  time: %lld us;\n", myTime);
    nd4j_printf("Old time: %lld us;\n", oldTime);

    delete []xOffsets;
    delete []yOffsets;
    delete []zOffsets;
    delete []buff;
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loops_3) {
    
    const uint N = 5;
    // const Nd4jLong dim0(1024), dim1(1024), dim2(1024);
    const Nd4jLong dim0(10), dim1(10), dim2(10);

    const Nd4jLong shapeInfo[2*3+4] = {3, dim0,dim1,dim2,  dim1*dim2,dim2,1,  8192,1,99};
    const Nd4jLong len = shape::length(shapeInfo);
    float* buff = new float[len];
    
    const Nd4jLong* shape   = shape::shapeOf(const_cast<Nd4jLong*>(shapeInfo));
    const Nd4jLong* strides = shape::stride(const_cast<Nd4jLong*>(shapeInfo));    

    // warm up
    for (int i = 0; i < 1000; ++i) 32*512;


    //***********************************
    //***********************************  
    auto timeStart = std::chrono::system_clock::now();    


    for (int i = 0; i < N; ++i)
    {
        Nd4jLong* idxX          = new Nd4jLong[3];
        Nd4jLong* idxY          = new Nd4jLong[3];
        Nd4jLong* idxZ          = new Nd4jLong[3];
        Nd4jLong* offsetPerDimX = new Nd4jLong[3];
        Nd4jLong* offsetPerDimY = new Nd4jLong[3];
        Nd4jLong* offsetPerDimZ = new Nd4jLong[3];
        memset(idxX, 0, sizeof(Nd4jLong) * 3);
        memset(idxY, 0, sizeof(Nd4jLong) * 3);
        memset(idxZ, 0, sizeof(Nd4jLong) * 3);

        PRAGMA_OMP_SIMD
        for (int k = 0; k < 3; ++k) {
            offsetPerDimX[k] = (shape[k] - 1) * strides[k];
            offsetPerDimY[k] = (shape[k] - 1) * strides[k];
            offsetPerDimZ[k] = (shape[k] - 1) * strides[k];
        }

        Nd4jLong initX(0), initY(0), initZ(0), offsetsX(0), offsetsY(0), offsetsZ(0);
        Nd4jLong rankMinusOne(3 - 1), jX(rankMinusOne), jY(rankMinusOne), jZ(rankMinusOne);

        // we do first iteration separately 
        buff[offsetsZ] = buff[offsetsX] * buff[offsetsY];
        uint e = 1;
        
        while (e < len) {

            // printf("%lld,  %lld,  %lld\n", jX, jY, jZ);
            if(shape[jX] == 1) { --jX; --jY; --jZ; continue; } 

            if(jX == rankMinusOne) { for(int l = 1; l < shape[jX]; ++l) {offsetsX += strides[jX]; ++e;} --jX; }            
            else if(idxX[jX] < shape[jX] - 1) {initX += strides[jX]; offsetsX = initX; ++idxX[jX]; jX = rankMinusOne; ++e;}
            else {initX -= offsetPerDimX[jX]; idxX[jX--] = 0;}

            if(jY == rankMinusOne) { for(int l = 1; l < shape[jY]; ++l) {offsetsY += strides[jY];} --jY; }            
            else if(idxY[jY] < shape[jY] - 1) {initY += strides[jY]; offsetsY = initY; ++idxY[jY]; jY = rankMinusOne; }
            else {initY -= offsetPerDimY[jY]; idxY[jY--] = 0;}

            if(jZ == rankMinusOne) { for(int l = 1; l < shape[jZ]; ++l) {offsetsZ += strides[jZ];} --jZ; }            
            else if(idxZ[jZ] < shape[jZ] - 1) {initZ += strides[jZ]; offsetsZ = initZ; ++idxZ[jZ]; jZ = rankMinusOne; }
            else {initZ -= offsetPerDimZ[jZ]; idxZ[jZ--] = 0;}

            buff[offsetsZ] = buff[offsetsX] * buff[offsetsY];
        }

        delete []idxX;            
        delete []idxY;            
        delete []idxZ;
        delete []offsetPerDimX;
        delete []offsetPerDimY;
        delete []offsetPerDimZ;            

    }

    auto timeEnd = std::chrono::system_clock::now();
    auto myTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N) .count();
    
    //***********************************
    //***********************************
    
    timeStart = std::chrono::system_clock::now();    

    // uint xShapeInfoCast[MAX_RANK];
    // uint yShapeInfoCast[MAX_RANK];
    // uint zShapeInfoCast[MAX_RANK];

    // bool canCastX = DataTypeUtils::castShapeInfo(shapeInfo, xShapeInfoCast);
    // bool canCastY = DataTypeUtils::castShapeInfo(shapeInfo, yShapeInfoCast);
    // bool canCastZ = DataTypeUtils::castShapeInfo(shapeInfo, zShapeInfoCast);

    // for (int i = 0; i < N; ++i)
    // {                 
    //     PRAGMA_OMP_PARALLEL_FOR_SIMD_COLLAPSE(1)
    //     for (uint i0 = 0; i0 < shape[0]; ++i0)
    //         for (uint i1 = 0; i1 < shape[1]; ++i1)
    //             for (uint i2 = 0; i2 < shape[2]; ++i2)
    //                 buff[i0*strides[0]+i1*strides[1]+i2*strides[2]] = buff[i0*strides[0]+i1*strides[1]+i2*strides[2]] * buff[i0*strides[0]+i1*strides[1]+i2*strides[2]];
    // }
     Nd4jLong *xOffsets, *yOffsets, *zOffsets;
    xOffsets = new Nd4jLong[len];
    yOffsets = new Nd4jLong[len];
    zOffsets = new Nd4jLong[len];
    
    for (int i = 0; i < N; ++i)
    {
            
        #pragma omp parallel sections
        {
            #pragma omp section
            {
    
                shape::calcOffsets(3, shape, strides, xOffsets);            
            } 
            #pragma omp section
            {
                
                shape::calcOffsets(3, shape, strides, yOffsets);
            } 
            #pragma omp section
            {
                
                shape::calcOffsets(3, shape, strides, zOffsets);
            }    
        }
     
    
        PRAGMA_OMP_PARALLEL_FOR_SIMD
        for (uint i = 0; i < len; i++) 
            buff[zOffsets[i]] = buff[xOffsets[i]] * buff[yOffsets[i]];
    } 
    timeEnd = std::chrono::system_clock::now();
    auto oldTime = std::chrono::duration_cast<std::chrono::microseconds> ((timeEnd - timeStart) / N).count();

        delete []xOffsets;
    delete []yOffsets;
    delete []zOffsets;
   
    nd4j_printf("My  time: %lld us;\n", myTime);
    nd4j_printf("Old time: %lld us;\n", oldTime);

    delete []buff;
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loops_4) {

    const uint N = 2;
    // const Nd4jLong dim0(256), dim1(256), dim2(256), dim3(256);
    const Nd4jLong dim0(10), dim1(10), dim2(10), dim3(10);
    NDArray x('c', {dim0, dim1, dim2, dim3});
    NDArray z('c', {dim0, dim2});

    x = 0.1;
    
    // warm up
    for (int i = 0; i < 1000; ++i) 32*512;

    
    auto timeStart = std::chrono::system_clock::now();    
    for (uint i = 0; i < N; ++i)
        x.reduceAlongDimension(reduce::Sum, &z, {1,3});
    auto timeEnd = std::chrono::system_clock::now();
    auto myTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart) / N) .count();
    nd4j_printf("My  time: %lld us;\n", myTime);
}

//////////////////////////////////////////////////////////////////////
TEST_F(PlaygroundTests, loops_5) {

    const uint N = 2;
    // const Nd4jLong dim0(1024), dim1(1024), dim2(256);
    const Nd4jLong dim0(10), dim1(10), dim2(10);
    NDArray x('c', {dim0, dim1, dim2});
    NDArray z('c', {dim0, dim1, dim2});

    // provide worst case 
    *shape::ews(x.shapeInfo()) = 0;
    *shape::ews(z.shapeInfo()) = 0;

    x = 0.1;
    
    // warm up
    for (int i = 0; i < 1000; ++i) 32*512;
    
    auto timeStart = std::chrono::system_clock::now();    
    for (uint i = 0; i < N; ++i)
        x.applyTransform(transform::Log, &z);
    auto timeEnd = std::chrono::system_clock::now();
    auto myTime = std::chrono::duration_cast<std::chrono::milliseconds> ((timeEnd - timeStart) / N) .count();
    nd4j_printf("My  time: %lld us;\n", myTime);
}
