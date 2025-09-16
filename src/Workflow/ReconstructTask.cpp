#include "glog/logging.h"
#include "ReconstructTask.h"
#include "WorkData.h"

void ReconstructTask::run()
{
	LOG(INFO) << "[Reconstructor] " << imageID;
	Reconstructor* reconstructor = m_data->getReconstructor();
	reconstructor->ReconstructImage(imageID);
}
