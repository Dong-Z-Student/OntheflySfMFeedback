#pragma once
#include "BaseTask.h"

class FeedbackTask : public BaseTask
{
	Q_OBJECT

public:
	explicit FeedbackTask(
		WorkData* data,
		QObject* parent = nullptr)
		: BaseTask(data, parent) {}
	void run() override;
signals:


public slots:


private:

};