#pragma once
#include <QMainWindow>
#include <filesystem>
#include "MeshViewerWidget.h"
#include "LogWidget.h"
#include "../Scene/meshing.h"
#include <thread>

using namespace std;
namespace fs = filesystem;

class TestMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	TestMainWindow(QWidget* parent = nullptr);

	MeshManager* meshManager;
	shared_ptr<Reconstruction> reconstruction;
	vector<fs::path> subFolders;
	size_t current_model_index;

	MeshViewerWidget* meshViewerWidget = nullptr;
	LogWidget* log_widget_;

	QToolBar* reconstruction_toolbar_;

	QDockWidget* dock_log_widget_;
	QDockWidget* dock_settings_widget_;

	vector<QAction*> actions;

	bool is_draw_mesh_lines_ = false;

	void CreateWidgets();
	void CreateToolbar();
	void CreateActions();

	void CreateMesh();
	void UpdateMesh();

	void RenderMesh();

signals:
	void meshReady();
};