#include "../Scene/mesh.h"
#include "../Scene/ReconstructionManager.h"
#include "FeedbackTask.h"
#include "WorkData.h"

void FeedbackTask::run()
{
	ReconstructionManager* manager = m_data->getModelManager();
	if (RawData* raw = m_data->getRawData()) {
		GenerateMeshModel(*raw);
	}
	emit taskFinished();
}
