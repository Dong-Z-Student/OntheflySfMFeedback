#include "../Feature/Bitmap.h"
#include "ExtractTask.h"
#include "WorkData.h"

// TODO: Òì³£´¦Àí
void ExtractTask::run()
{
	m_data->setImageStatus(imagePath, CImageStatusFlag::CExtracting);
	Timer ExtractionTimer;
	ExtractionTimer.Start();
	CSIFTExtractionOptions options;
	CSIFTGPUExtractor* SIFTGPUExtractor = new CSIFTGPUExtractor(options);
	FeatureKeypoints keypoints;
	FeatureDescriptors descriptors;
	LOG(INFO) << "[Extraction] Running sift extract: " << imagePath;
	Bitmap bitmap = m_data->ReadBitmap(imageid);
	if (SIFTGPUExtractor->Extract(bitmap, keypoints, descriptors))
	{
		const size_t imageID = m_data->getImageID(imagePath);
		const size_t cameraID = m_data->DatabaseReadImage(imageID).CameraId();
		ScaleKeypoints(bitmap, m_data->DatabaseReadCamera(cameraID), &keypoints);
		m_data->DatabaseWriteKeypoints(imageID, keypoints);
		m_data->DatabaseWriteDescriptors(imageID, descriptors);
		m_data->setImageStatus(imagePath, CImageStatusFlag::CExtracted);

		const size_t ExtractionTime = ExtractionTimer.ElapsedMicroSeconds() / 1000;
		m_data->SIFTExtractionTimes.at(imageID) += ExtractionTime;

		LOG(INFO) << "[Extraction] Image " << imagePath << " extracted successed!";

		delete SIFTGPUExtractor;
		m_data->ReleaseBitmap(imageid);


	}
}
