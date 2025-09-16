#include "../Base/timer.h"
#include "glog/logging.h"
#include "MatchTask.h"
#include "WorkData.h"

void MatchTask::run()
{
	image_t imageID1 = m_data->getImageID(imagePath.toStdString());
	image_t imageID2 = m_data->getImageID(secondImagePath.toStdString());

	Timer SIFTMatchingTimer;
	SIFTMatchingTimer.Start();

	CSIFTMatchingOptions options;
	const FeatureDescriptors& descriptors1 = m_data->DatabaseReadDescriptors(imageID1);
	const FeatureDescriptors& descriptors2 = m_data->DatabaseReadDescriptors(imageID2);
	CHECK(descriptors1.cols() == 128 && descriptors2.cols() == 128);

	FeatureMatches matches;
	CSIFTGPUMatcher* SIFTGPUMatcher = new CSIFTGPUMatcher(options);
	CSIFTGPUMatcher::isUploadDescriptors1 = (CSIFTGPUMatcher::lastDescriptors1Index != imageID1);
	CSIFTGPUMatcher::isUploadDescriptors2 = (CSIFTGPUMatcher::lastDescriptors2Index != imageID2);
	matches = SIFTGPUMatcher->Match(descriptors1, descriptors2);
	CSIFTGPUMatcher::lastDescriptors1Index = imageID1;
	CSIFTGPUMatcher::lastDescriptors2Index = imageID2;

	if (!matches.empty())
	{
		m_data->DatabaseWriteMatches(imageID1, imageID2, matches);
	}
	return true;
	if (CWorkflow::SIFTMatch(*database, matchingPair.first, matchingPair.second, options))
	{
		const size_t numMatches = (database->ExistsMatches(matchingPair.first, matchingPair.second) ? database->ReadMatches(matchingPair.first, matchingPair.second).size() : 0);
		LOG(INFO) << "[MatchGPU] " << matchingPair.first << " - " << matchingPair.second << ": " << numMatches;

		const size_t SIFTMatchingTime = SIFTMatchingTimer.ElapsedMicroSeconds() / 1000;
		SIFTMatchingTimes.at(matchingPair.second) += SIFTMatchingTime;

		estimateTasks.push(matchingPair);
	}
	else
	{
		LOG(INFO) << "SIFTMatchGPUTasks warning!";
		reconstructionTasks.insert(matchingPair.second);
	}

	delete SIFTGPUMatcher;
}
