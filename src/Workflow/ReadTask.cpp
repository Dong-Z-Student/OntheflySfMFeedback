#include "ReadTask.h"
#include "WorkData.h"

#include <string>

using namespace std;

void ReadTask::run()
{
	Camera camera;
	std::string cameraModel;
	Bitmap bitmap;
	Image image;
	ImageReader* imageReader = new ImageReader();

	Timer imageReaderTimer;
	imageReaderTimer.Start();

	if (imageReader->Read(imagePath, camera, cameraModel, image, bitmap) == ImageReader::Status::SUCCESS)
	{
		camera_t cameraID = m_data->DatabaseWriteCamera(camera);
		camera.SetCameraId(cameraID);
		image.SetCameraId(cameraID);
		image_t imageID = m_data->DatabaseWriteImage(image);

		m_data->addImageMap(imageID, imagePath);

		const size_t readTime = imageReaderTimer.ElapsedMicroSeconds() / 1000;
		m_data->SIFTExtractionTimes[imageID] = readTime;
		m_data->globalFeatureExtractionTimes[imageID] = 0;
		m_data->globalFeatureRetrievalTimes[imageID] = 0;
		m_data->SIFTMatchingTimes[imageID] = 0;
		m_data->geometricVerificationTimes[imageID] = 0;

		m_data->WriteBitmap(bitmap, imageID);

		m_data->setImageStatus(imagePath, CImageStatusFlag::CRead);
		QString imagePathQstr = QString::fromUtf8(imagePath.c_str());
		emit readTaskDone(imageID, imagePathQstr);
	}
	delete imageReader;
}