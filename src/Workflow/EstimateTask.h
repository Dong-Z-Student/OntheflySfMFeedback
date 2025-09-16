#pragma once
#include "BaseTask.h"

class EstimateTask : public BaseTask
{
	Q_OBJECT

public:
	explicit EstimateTask(
		WorkData* data,
		quint32 p_imageID1,
		quint32 p_imageID2,
		QObject* parent = nullptr)
		: BaseTask(data, parent)
		, imageID1(p_imageID1)
		, imageID2(p_imageID2) {}
	void run() override;

signals:
	void estimateTaskFinished(quint32 p_imageID1, quint32 p_imageID2);
	void estimateAllFinished(quint32 p_imageID);
	void estimateTaskFailed();

private:
	quint32 imageID1;
	quint32 imageID2;
};