#pragma once
#include "BaseTask.h"

class ReadTask : public BaseTask
{
	Q_OBJECT

public:
	explicit ReadTask(WorkData* data, std::string& p_imagePath, QObject* parent = nullptr)
		: BaseTask(data, parent),
		imagePath(p_imagePath) {}

	void run() override;

signals:
	void readTaskDone(quint32 imageID, QString p_imagePath);

private:
	std::string imagePath;
};
