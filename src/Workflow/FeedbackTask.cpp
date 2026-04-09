#include "../Scene/mesh.h"
#include "../Scene/ReconstructionManager.h"
#include "FeedbackTask.h"
#include "WorkData.h"

void FeedbackTask::run()
{
	ReconstructionManager* manager = m_data->getModelManager();
	if (RawData* raw = m_data->getRawData()) {
		m_data->m_feedback_lock = true;
		GenerateMeshModel(*raw);
		m_data->m_feedback_lock = false;
	}
	emit taskFinished();
}
