#pragma once
#include "BaseTask.h"

class MatchTask : public BaseTask
{
	Q_OBJECT

public:
	explicit MatchTask(
		WorkData* data,
		QString p_imagePath,
		QString p_secondImagePath,
		QObject* parent = nullptr)
		: BaseTask(data, parent)
		, imagePath(p_imagePath)
		, secondImagePath(p_secondImagePath) {}
	~MatchTask() override {

	}
	void run() override;
signals:
	void matchTaskFinished(QString p_imagePath, QString p_secondImagePath);

private:
	QString imagePath;
	QString secondImagePath;
};