/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <TestImpactFramework/TestImpactFileUtils.h>

#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Run/TestImpactTestRunSerializer.h>
#include <TestEngine/Run/TestImpactTestRunner.h>

#include <AzCore/IO/SystemFile.h>

namespace TestImpact
{
    TestRunner::TestRunner(size_t maxConcurrentRuns)
        : JobRunner(maxConcurrentRuns)
    {
    }

    AZStd::pair<ProcessSchedulerResult, AZStd::vector<TestRunner::Job>> TestRunner::RunTests(
        const AZStd::vector<JobInfo>& jobInfos,
        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
        AZStd::optional<ClientJobCallback> clientCallback)
    {
        const auto payloadGenerator = [this](const JobDataMap& jobDataMap)
        {
            PayloadMap<Job> runs;
            for (const auto& [jobId, jobData] : jobDataMap)
            {
                const auto& [meta, jobInfo] = jobData;
                if (meta.m_result == JobResult::ExecutedWithSuccess || meta.m_result == JobResult::ExecutedWithFailure)
                {
                    try
                    {
                        runs[jobId] = TestRun(GTest::TestRunSuitesFactory(ReadFileContents<TestEngineException>(jobInfo->GetRunArtifactPath())), meta.m_duration.value());
                    }
                    catch (const Exception& e)
                    {
                        AZ_Printf("RunTests", AZStd::string::format("%s\n", e.what()).c_str());
                        runs[jobId] = AZStd::nullopt;
                    }
                }
            }

            return runs;
        };

        return ExecuteJobs(
            jobInfos,
            payloadGenerator,
            StdOutputRouting::None,
            StdErrorRouting::None,
            runTimeout,
            runnerTimeout,
            clientCallback,
            AZStd::nullopt);
    }
} // namespace TestImpact