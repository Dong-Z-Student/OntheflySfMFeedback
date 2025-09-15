// WorkFlow.h
#ifndef WORKFLOW_H
#define WORKFLOW_H

#include "../Scene/Database.h"
#include "ExtractTask.h"
#include "PythonTask.h"
#include "ReadTask.h"
#include "Reconstructor.h"
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

	void setData(Reconstructor* p_reconstructor, Database* p_database);
	void LoadPython();

private:
	WorkData* m_workData;      // 共享数据
	QThreadPool* m_read_threadPool; // 线程池
	QThreadPool* m_extract_threadPool;
	QThreadPool* m_python_threadPool;


public slots:
	void onProcessNewImage(const QString& value); // 传入影像，启动工作流
	void onReadTaskFinished(quint32 imageid, QString p_imagePath);
};

#endif // WORKFLOW_H
