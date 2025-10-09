// WorkFlow.h
#ifndef WORKFLOW_H
#define WORKFLOW_H

#include "../Scene/Database.h"
#include "../Scene/ReconstructionManager.h"
#include "EstimateTask.h"
#include "ExtractTask.h"
#include "FeedbackTask.h"
#include "MatchTask.h"
#include "PythonTask.h"
#include "ReadTask.h"
#include "ReconstructTask.h"
#include "RetrieveTask.h"
#include <QMutex>
#include <QObject>
#include <QString>
#include <QtConcurrent>
#include <QThread>
#include <QThreadPool>

class WorkFlow : public QObject
{
	Q_OBJECT

public:
	explicit WorkFlow(QObject* parent = nullptr);
	~WorkFlow();

	void setData(Database* p_database, ReconstructionManager* p_modelManager);
	void LoadPython();

signals:
	void readImageFinished(QString p_imagePath);
	void reconstructUpdated(); // refresh the model in model viewer
	void changeLastRegistedImageID(quint32 imageID);

public slots:
	void onProcessNewImage(const QString& value); // 传入影像，启动工作流
	void onReadTaskFinished(quint32 imageid, QString p_imagePath);
	void onLocalFeatureExtractFinished(QString p_imagePath);
	void onGlobalFeatureExtractFinished(QString p_imagePath);
	void onRetrieveTaskFinished(quint32 imageID1, quint32 imageID2);
	void onMatchTaskFinished(quint32 imageID1, quint32 imageID2);
	void onEstimateFinished(quint32 imageID1, quint32 imageID2);
	void onRegistedInitialImagePair(quint32 imageID1, quint32 imageID2);
	void onRegistedSuccess(quint32 imageID);
	void onFeedbackFinished();

private:
	WorkData* m_workData;      // 共享数据
	QThreadPool* m_read_threadPool; // 线程池
	QThreadPool* m_extract_threadPool;
	QThreadPool* m_gextract_threadPool;
	QThreadPool* m_retrieve_threadPool;
	QThreadPool* m_match_threadPool;
	QThreadPool* m_estimate_threadPool;
	QThreadPool* m_reconstruct_threadPool;
	QThreadPool* m_feedback_threadPool;

	QMutex mu_reconstructTasks; // 分发任务加锁

	bool ReconstructImage(quint32 imageID);
	void GenerateMeshModel();
};

#endif // WORKFLOW_H
