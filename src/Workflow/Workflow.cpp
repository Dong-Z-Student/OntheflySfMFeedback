#include "glog/logging.h"
#include "WorkData.h"
#include "WorkFlow.h"

using namespace std;

WorkFlow::WorkFlow(QObject* parent)
	: QObject(parent)
	, m_workData(new WorkData(this))
	, m_read_threadPool(new QThreadPool(this))
	, m_extract_threadPool(new QThreadPool(this))
	, m_gextract_threadPool(new QThreadPool(this))
	, m_retrieve_threadPool(new QThreadPool(this))
	, m_match_threadPool(new QThreadPool(this))
	, m_estimate_threadPool(new QThreadPool(this))
	, m_reconstruct_threadPool(new QThreadPool(this))
{
	LoadPython();
	m_read_threadPool->setMaxThreadCount(1);
	m_extract_threadPool->setMaxThreadCount(2);
	m_gextract_threadPool->setMaxThreadCount(1);
	m_retrieve_threadPool->setMaxThreadCount(2);
	m_match_threadPool->setMaxThreadCount(3);
	m_estimate_threadPool->setMaxThreadCount(8);
	m_reconstruct_threadPool->setMaxThreadCount(1);
}

WorkFlow::~WorkFlow()
{
	delete m_workData;
	delete m_read_threadPool;
	delete m_extract_threadPool;
	delete m_gextract_threadPool;
	delete m_retrieve_threadPool;
	delete m_match_threadPool;
	delete m_reconstruct_threadPool;
}

void WorkFlow::setData(Database* p_database, ReconstructionManager* p_modelManager)
{
	m_workData->Initial(p_database, p_modelManager);
}

void WorkFlow::LoadPython()
{
	QtConcurrent::run([this]() {
		QThread::sleep(1);
		PyLoader* pyloader = new PyLoader();
		size_t retrievalTopN = 30;
#ifndef NOT_USE_GLOBAL_FEATURES
		CGlobalFeatureExtractor* globalFeatureExtractor = new CGlobalFeatureExtractor();
		CGlobalFeatureRetriever* globalFeatureRetriever = new CGlobalFeatureRetriever(m_workData->getDatabase(), retrievalTopN);
#endif
		m_workData->setGlobalFeatureFunc(globalFeatureExtractor, globalFeatureRetriever);
		}
	);
}

void WorkFlow::onProcessNewImage(const QString& value) {
	ReadTask* m_readTask = new ReadTask(m_workData, value, this);
	connect(m_readTask, &ReadTask::readTaskDone, this, &WorkFlow::onReadTaskFinished);
	m_read_threadPool->start(m_readTask);
}

void WorkFlow::onReadTaskFinished(quint32 imageid, QString p_imagePath)
{
	emit readImageFinished(p_imagePath);
	ExtractTask* m_extractTask = new ExtractTask(m_workData, p_imagePath, imageid, this);
	connect(m_extractTask, &ExtractTask::localExtractFinished, this, &WorkFlow::onLocalFeatureExtractFinished);
	m_extract_threadPool->start(m_extractTask);
}

void WorkFlow::onLocalFeatureExtractFinished(QString p_imagePath)
{
	PythonTask* m_gextractTask = new PythonTask(m_workData, p_imagePath, PythonTaskMode::GlobalFeatureExtract, this);
	connect(m_gextractTask, &PythonTask::globalFeatureExtractFinished, this, &WorkFlow::onGlobalFeatureExtractFinished);
	m_gextract_threadPool->start(m_gextractTask);
}

void WorkFlow::onGlobalFeatureExtractFinished(QString p_imagePath)
{
	RetrieveTask* m_retrieveTask = new RetrieveTask(m_workData, p_imagePath, this);
	connect(m_retrieveTask, &RetrieveTask::retrieveTaskFinished, this, &WorkFlow::onRetrieveTaskFinished);
	m_retrieve_threadPool->start(m_retrieveTask);
}

void WorkFlow::onRetrieveTaskFinished(quint32 imageID1, quint32 imageID2) {
	MatchTask* m_matchTask = new MatchTask(m_workData, imageID1, imageID2, this);
	connect(m_matchTask, &MatchTask::matchTaskFinished, this, &WorkFlow::onMatchTaskFinished);
	m_match_threadPool->start(m_matchTask);
}

void WorkFlow::onMatchTaskFinished(quint32 imageID1, quint32 imageID2)
{
	EstimateTask* m_estimateTask = new EstimateTask(m_workData, imageID1, imageID2);
	connect(m_estimateTask, &EstimateTask::estimateAllFinished, this, &WorkFlow::onEstimateFinished);
	m_estimate_threadPool->start(m_estimateTask);
}

void WorkFlow::onEstimateFinished(quint32 imageID) {
	ReconstructImage(imageID);
}

void WorkFlow::onReconstructUpdated(quint32 imageID)
{
	emit reconstructUpdated();
}

bool WorkFlow::ReconstructImage(quint32 imageID)
{
	if (m_workData->estimatedImageNum == 20) {
		ReconstructTask* m_reconstructTask = new ReconstructTask(m_workData, imageID, -1, ReconstructType::AddModel, this);
		connect(m_reconstructTask, &ReconstructTask::reconstructionInitial, this, &WorkFlow::onReconstructUpdated);
		connect(m_reconstructTask, &ReconstructTask::registedSuccess, this, &WorkFlow::onReconstructUpdated);
		m_reconstruct_threadPool->start(m_reconstructTask);
	}
	//if (m_workData->estimatedImageNum > 20) {
	//	ReconstructTask* m_reconstructTask = new ReconstructTask(m_workData, imageID, 0, ReconstructType::AddImage, this);
	//	connect(m_reconstructTask, &ReconstructTask::reconstructTaskFinished, this, &WorkFlow::onReconstructUpdated);
	//	m_reconstruct_threadPool->start(m_reconstructTask);
	//}
	return false;
}

