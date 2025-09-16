#include "../Base/logging.h"
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
{
	LoadPython();
	m_read_threadPool->setMaxThreadCount(1);
	m_extract_threadPool->setMaxThreadCount(3);
	m_gextract_threadPool->setMaxThreadCount(1);
	m_retrieve_threadPool->setMaxThreadCount(4);
}

WorkFlow::~WorkFlow()
{
	delete m_workData;
	delete m_read_threadPool;
	delete m_extract_threadPool;
	delete m_gextract_threadPool;
	delete m_retrieve_threadPool;
}

void WorkFlow::setData(Reconstructor* p_reconstructor, Database* p_database)
{
	m_workData->Initial(p_reconstructor, p_database);
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
	m_retrieve_threadPool->start(m_retrieveTask);
}

