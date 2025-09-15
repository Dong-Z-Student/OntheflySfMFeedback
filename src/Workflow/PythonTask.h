#pragma once
#include "BaseTask.h"

class PythonTask : public BaseTask
{
	Q_OBJECT

public:
	explicit PythonTask(WorkData* data,
		QString p_imagepath,
		QObject* parent = nullptr)
		: BaseTask(data, parent)
		, imagePathQstr(p_imagepath) {}

	void run() override;


private:
	QString imagePathQstr;
};