#include "../Scene/mesh.h"
#include "../Scene/ReconstructionManager.h"
#include "FeedbackTask.h"
#include "WorkData.h"

void FeedbackTask::run()
{
	ReconstructionManager* manager = m_data->getModelManager();
	if (RawData* raw = m_data->getRawData()) {
		m_data->m_feedback_lock = true;
		std::shared_ptr<RenderData> newRenderData = std::make_shared<RenderData>();
		GenerateMeshModel(*raw, *newRenderData);
		m_data->render_data = newRenderData;
		m_data->m_feedback_lock = false;
	}
	emit taskFinished();
}
