#pragma once
#include "BaseTask.h"

class ReconstructTask : public BaseTask
{
	Q_OBJECT

public:
	explicit ReconstructTask(
		WorkData* data,
		quint32 p_imageID,
		QObject* parent = nullptr)
		: BaseTask(data, parent)
		, imageID(p_imageID) {}
	void run() override;

signals:
	void reconstructTaskFinished(quint32 p_imageID);

private:
	quint32 imageID;
};