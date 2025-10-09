#include "../Scene/meshing.h"
#include "../Scene/ReconstructionManager.h"
#include "FeedbackTask.h"
#include "WorkData.h"

void FeedbackTask::run()
{
	ReconstructionManager* modelManager = m_data->getModelManager();
	std::shared_ptr<Reconstruction> model = modelManager->Get(0);
	std::shared_ptr<MeshManager> meshManager = std::make_shared<MeshManager>();
	meshManager->CreateMesh(*model);
	//meshManager->GenerateRenderMeshData(*model);
	//m_data->m_render_data = meshManager->GetRenderingData();

	emit taskFinished();
}
