#include "../Base/logging.h"
#include "WorkData.h"
#include "WorkFlow.h"

using namespace std;

WorkFlow::WorkFlow(QObject* parent)
	: QObject(parent)
	, m_workData(new WorkData(this))
	, m_read_threadPool(new QThreadPool(this))
	, m_extract_threadPool(new QThreadPool(this))
	, m_python_threadPool(new QThreadPool(this))
{
	LoadPython();
	m_read_threadPool->setMaxThreadCount(1);
	m_extract_threadPool->setMaxThreadCount(3);
	m_python_threadPool->setMaxThreadCount(1);
}

WorkFlow::~WorkFlow()
{
	delete m_workData;
	delete m_read_threadPool;
	delete m_extract_threadPool;
	delete m_python_threadPool;
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
		m_workData->setPythonFunc(globalFeatureExtractor, globalFeatureRetriever);
		}
	);
}

void WorkFlow::onProcessNewImage(const QString& value) {
	std::string valuestr = value.toStdString();
	ReadTask* m_readTask = new ReadTask(m_workData, valuestr, this);
	connect(m_readTask, &ReadTask::readTaskDone, this, &WorkFlow::onReadTaskFinished);
	m_read_threadPool->start(m_readTask);
}

void WorkFlow::onReadTaskFinished(quint32 imageid, QString p_imagePath)
{
	std::string imagePathStr = p_imagePath.toUtf8().constData();
	ExtractTask* m_extractTask = new ExtractTask(m_workData, imagePathStr, imageid, this);
	m_extract_threadPool->start(m_extractTask);

	PythonTask* m_pythonTask = new PythonTask(m_workData, p_imagePath, this);
	m_python_threadPool->start(m_pythonTask);
}