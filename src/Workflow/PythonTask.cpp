#include "../Base/timer.h"
#include "../Base/types.h"
#include "../Feature/FeatureExtractor.h"
#include "../Feature/PyLoader.h"
#include "glog/logging.h"
#include "PythonTask.h"
#include "WorkData.h"

void PythonTask::run()
{
	PythonThreadLocker lock;

	Timer globalFeatureExtractionTimer;
	globalFeatureExtractionTimer.Start();
	std::string imagePath = imagePathQstr.toUtf8().constData();
	GlobalFeature globalFeature;
	CGlobalFeatureExtractor* globalFeatureExtractor = m_data->getGlobalFeatureExtractor();
	bool extractFlag = globalFeatureExtractor->Extract(imagePath, globalFeature);
	if (extractFlag)
	{
		const size_t imageID = m_data->getImageID(imagePath);
		m_data->DatabaseWriteGlobalFeature(imageID, globalFeature);
		//retrievalTasks.push(imageID);

		const size_t globalFeatureExtractionTime = globalFeatureExtractionTimer.ElapsedMicroSeconds() / 1000;
		m_data->globalFeatureExtractionTimes.at(imageID) = globalFeatureExtractionTime;

		LOG(INFO) << "[Global feature] Image " << imagePath << " extracted successed!";
	}
	else
	{
		LOG(INFO) << "[Global feature] Image " << imagePath << " extracted failed!";
	}
}
