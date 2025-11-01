#include "../Scene/meshing.h"
#include "../Scene/ReconstructionManager.h"
#include "FeedbackTask.h"
#include "WorkData.h"

void FeedbackTask::run()
{
	std::shared_ptr<MeshManager> meshManager = std::make_shared<MeshManager>();

	{
		QReadLocker locker(&(m_data->m_models_lock[modelID]));
		meshManager->CopyFromReconstruction(*(m_data->getModelManager()->Get(modelID)));
	}
	meshManager->CreateMesh();
	meshManager->GenerateRenderMeshData();
	m_data->render_data = meshManager->GetRenderingData();

	emit taskFinished();
}
