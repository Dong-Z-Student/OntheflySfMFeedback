#include "../Scene/meshing.h"
#include "../Scene/ReconstructionManager.h"
#include "FeedbackTask.h"
#include "WorkData.h"

void FeedbackTask::run()
{
	ReconstructionManager* modelManager = m_data->getModelManager();
	std::shared_ptr<Reconstruction> model = modelManager->Get(modelID);
	std::shared_ptr<MeshManager> meshManager = std::make_shared<MeshManager>();

	{
		QReadLocker locker(&(m_data->getModelLock(modelID)));
		meshManager->CopyFromReconstruction(*model);
	}
	meshManager->CreateMesh();
	meshManager->GenerateRenderMeshData();
	m_data->render_data = meshManager->GetRenderingData();

	emit taskFinished();
}
