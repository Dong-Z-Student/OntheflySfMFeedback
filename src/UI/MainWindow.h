#pragma once
#include "../Workflow/Receiver.h"
#include "../Workflow/Workflow.h"
#include "ImageListWidget.h"
#include "LogWidget.h"
#include "MatchMatrixWidget.h"
#include "ModelViewerWidget.h"
#include "ProjectWidget.h"
#include "ReconstructionManagerWidget.h"
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	void StartReceive();
	void StopReceive();
	void ProcessImage(const std::string& newImagePath);

protected:
	void closeEvent(QCloseEvent* event) override;

private slots:
	void NewProject();
	void ExportProject();
	void ChangeCurrentModel();
	void ShowMatchMatrix();
	void ChangeSelectModel();
	void RenderNow();
	void NewProject_SLOT();
	void onTabChanged(int index);

	void addNewImageInWidget(QString p_imagePath);

private:
	void SetupWidgets();
	void SetupActions();
	void SetupConnections() const;
	void CreateMenus();
	void CreateToolbar();
	void CreateStatusbar();
	void RunFunc();
	void ExportRunningProject();
	void ExportModelOnRegisted();

	Receiver* receiver;
	Database* database;
	ReconstructionManager* reconstructionManager;
	WorkFlow* workflow;
	ModelViewerWidget* modelViewerWidget;
	ReconstructionManagerWidget* reconstructionManagerWidget;
	ImageListWidget* imageListWidget;
	LogWidget* logWidget;
	ProjectWidget* projectWidget;
	ShowMatchMatrixWidget* showMatchMatrixWidget;
	QTabWidget* tabWidget;
	QDockWidget* imageListWindowDock;
	QDockWidget* logWindowDock;
	QLabel* statusLabel;
	QAction* newProjectAction;
	QAction* exportProjectAction;
	QAction* startReceiveAction;
	QAction* stopReceiverAction;
	QAction* renderNowAction;
	QAction* showMatchMatrixAction;
	QMenu* projectMenu;
	QMenu* transmitMenu;
	QToolBar* projectToolBar;
	QToolBar* transmitToolBar;
	QToolBar* reconstructToolBar;
	QToolBar* feedbackToolBar;
	QCheckBox* isTrackModelCheckBox;
	bool isForcedRender = false;
	std::mutex renderMutex;
};