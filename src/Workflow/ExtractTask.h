#pragma once
#include "BaseTask.h"

class ExtractTask : public BaseTask
{
	Q_OBJECT

public:
	explicit ExtractTask(WorkData* data,
		std::string& p_imagePath,
		quint32 p_imageid,
		QObject* parent = nullptr)
		: BaseTask(data, parent)
		, imagePath(p_imagePath)
		, imageid(p_imageid) {}

	void run() override;


private:
	std::string imagePath;
	quint32 imageid;
};