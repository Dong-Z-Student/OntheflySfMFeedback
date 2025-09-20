#include "../Base/config.h"
#include "../Base/macros.h"
#include "../Base/misc.h"
#include "MainWindow.h"
#include <filesystem>
#include <QSettings>
#include <QString>
#include <QTabWidget>

using namespace std;

// ================= 工具函数 =================
std::string ToLowerCase(const std::string& str) {
	std::string lowerCaseStr = str;
	std::transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return lowerCaseStr;
}

std::vector<std::string> GetImageFileList(const std::string& path) {
	std::vector<std::string> fileList;
	std::filesystem::path dirPath(path);
	const std::vector<std::string> imageExtensions = { ".jpg", ".jpeg", ".png", ".bmp", ".gif" };

	if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			if (entry.is_regular_file()) {
				std::filesystem::path filePath = entry.path();
				std::string extension = ToLowerCase(filePath.extension().string());
				if (std::find(imageExtensions.begin(), imageExtensions.end(), extension) != imageExtensions.end()) {
					fileList.push_back(filePath.string());
				}
			}
		}
	}
	return fileList;
}

QString StdString2QString2(std::string string) {
	return QString::fromLocal8Bit(string.data());
}

int Add(int currentTime) {
	int hours = currentTime / 10000;
	int minutes = (currentTime / 100) % 100;
	int seconds = currentTime % 100;

	seconds++;
	if (seconds == 60) {
		seconds = 0;
		minutes++;
		if (minutes == 60) {
			minutes = 0;
			hours++;
			if (hours == 24) {
				hours = 0;
			}
		}
	}
	int newTime = hours * 10000 + minutes * 100 + seconds;
	return newTime;
}

// ================= 构造/析构与事件 =================
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	receiver = new Receiver(this);
	database = new Database();
	reconstructionManager = new ReconstructionManager();
	workflow = new WorkFlow(this);
	workflow->setData(database, reconstructionManager);

	SetupWidgets();
	SetupActions();
	CreateMenus();
	CreateToolbar();
	CreateStatusbar();
	SetupConnections();
}

MainWindow::~MainWindow()
{
	delete receiver;
	delete database;
	delete reconstructionManager;
	delete workflow;
}

void MainWindow::closeEvent(QCloseEvent* event) {

	receiver->Stop();
	event->accept();
}

// ================= 初始化相关 =================
void MainWindow::SetupWidgets() {
	resize(600, 600);
	setWindowTitle(tr("On-the-fly SfM"));
	setWindowIcon(QIcon(":/media/WindowIcon.png"));

	tabWidget = new QTabWidget(this);
	modelViewerWidget = new ModelViewerWidget(this);
	modelViewerWidget->SetDatabase(database);
	tabWidget->addTab(modelViewerWidget, "Model Viewer");
	setCentralWidget(tabWidget);

	reconstructionManagerWidget = new ReconstructionManagerWidget(this, std::shared_ptr<const ReconstructionManager>(reconstructionManager));
	reconstructionManagerWidget->setFixedWidth(270);
	reconstructionManagerWidget->Update();
	connect(reconstructionManagerWidget, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::ChangeSelectModel);

	imageListWindowDock = new QDockWidget(tr("Image Preview"), this);
	imageListWindowDock->setMinimumHeight(100);
	imageListWindowDock->setMinimumWidth(250);
	imageListWidget = new ImageListWidget(imageListWindowDock, database);
	imageListWindowDock->setWidget(imageListWidget);
	addDockWidget(Qt::RightDockWidgetArea, imageListWindowDock);

	logWindowDock = new QDockWidget(tr("Log"), this);
	logWidget = new LogWidget(this);
	logWindowDock->setWidget(logWidget);
	addDockWidget(Qt::BottomDockWidgetArea, logWindowDock);
	logWindowDock->show();
	logWindowDock->raise();

	projectWidget = new ProjectWidget(this);
	connect(projectWidget, &ProjectWidget::NewProject_SIGNAL, this, &MainWindow::NewProject_SLOT);

	isTrackModelCheckBox = new QCheckBox(tr("Track Workflow"));
	isTrackModelCheckBox->setChecked(true);

	showMatchMatrixWidget = new ShowMatchMatrixWidget(this, database);
}

void MainWindow::SetupActions() {
	newProjectAction = new QAction(QIcon(":/media/project-new.png"), tr("New Project"), this);
	connect(newProjectAction, &QAction::triggered, this, &MainWindow::NewProject);

	exportProjectAction = new QAction(QIcon(":/media/project-save-as.png"), tr("Export Project"), this);
	connect(exportProjectAction, &QAction::triggered, this, &MainWindow::ExportProject);
	exportProjectAction->setEnabled(false);

	startReceiveAction = new QAction(QIcon(":/media/reconstruction-start.png"), tr("Start Receive"), this);
	connect(startReceiveAction, &QAction::triggered, this, &MainWindow::StartReceive);

	stopReceiverAction = new QAction(QIcon(":/media/reconstruction-pause.png"), tr("Stop Receive"), this);
	connect(stopReceiverAction, &QAction::triggered, this, &MainWindow::StopReceive);
	stopReceiverAction->setEnabled(false);

	renderNowAction = new QAction(this);
	connect(renderNowAction, &QAction::triggered, this, &MainWindow::RenderNow);

	//reconstructor->AddCallback(
	//	Reconstructor::INITIAL_IMAGE_PAIR_REG_CALLBACK, [this]() { renderNowAction->trigger(); });
	//reconstructor->AddCallback(
	//	Reconstructor::NEXT_IMAGE_REG_CALLBACK, [this]() { renderNowAction->trigger(); });
	//reconstructor->AddCallback(
	//	Reconstructor::CHANGE_CURRENT_MODEL_CALLBACK, [this]() { ChangeCurrentModel(); });

	showMatchMatrixAction = new QAction(QIcon(":/media/match-matrix.png"), tr("Show Match Matrix"), this);
	connect(showMatchMatrixAction, &QAction::triggered, this, &MainWindow::ShowMatchMatrix);
}

void MainWindow::CreateMenus() {
	projectMenu = new QMenu(tr("Project"), this);
	projectMenu->addAction(newProjectAction);
	projectMenu->addAction(exportProjectAction);
	menuBar()->addAction(projectMenu->menuAction());

	transmitMenu = new QMenu(tr("Transmit"), this);
	transmitMenu->addAction(startReceiveAction);
	transmitMenu->addAction(stopReceiverAction);
	menuBar()->addAction(transmitMenu->menuAction());
}

void MainWindow::CreateToolbar() {
	projectToolBar = addToolBar(tr("Project"));
	projectToolBar->addAction(newProjectAction);
	projectToolBar->addAction(exportProjectAction);
	projectToolBar->setIconSize(QSize(16, 16));
	projectToolBar->setMovable(false);

	transmitToolBar = addToolBar(tr("Transmit"));
	transmitToolBar->addAction(startReceiveAction);
	transmitToolBar->addAction(stopReceiverAction);
	transmitToolBar->setIconSize(QSize(16, 16));
	transmitToolBar->setMovable(false);

	reconstructToolBar = addToolBar(tr("Reconstruct"));
	reconstructToolBar->addAction(showMatchMatrixAction);
	reconstructToolBar->addWidget(reconstructionManagerWidget);
	reconstructToolBar->setIconSize(QSize(16, 16));
	reconstructToolBar->addSeparator();
	reconstructToolBar->addWidget(isTrackModelCheckBox);
	reconstructToolBar->setMovable(false);

	feedbackToolBar = addToolBar(tr("Feedback"));
	feedbackToolBar->setIconSize(QSize(16, 16));
	feedbackToolBar->setMovable(false);
}

void MainWindow::CreateStatusbar() {
	QFont font;
	font.setPointSize(11);
	statusLabel = new QLabel("0 Images - 0 Points", this);
	statusLabel->setFont(font);
	statusLabel->setAlignment(Qt::AlignCenter);
	statusBar()->addWidget(statusLabel, 1);
}

void MainWindow::SetupConnections() const {
	connect(receiver, &Receiver::newDataReceived, workflow, &WorkFlow::onProcessNewImage);
	connect(workflow, &WorkFlow::readImageFinished, this, &MainWindow::addNewImageInWidget);
	connect(workflow, &WorkFlow::reconstructUpdated, this, &MainWindow::RenderNow);
}

// ================= 业务逻辑 =================
void MainWindow::RunFunc() {
	const std::string baseImagePath = Database::imageDir;
	const vector<std::string> images = GetImageFileList(baseImagePath);

	QSettings settings("HKEY_CURRENT_USER\\Software\\RTPS\\CamFiTransmit", QSettings::NativeFormat);
	for (int i = 0; i < images.size(); i++) {
		const QString imagePath = StdString2QString2(images[i]);
		settings.setValue("NewImage", imagePath);

		this_thread::sleep_for(chrono::milliseconds(500));
	}
}

void MainWindow::StartReceive() {
	receiver->Start();
	startReceiveAction->setEnabled(false);
	stopReceiverAction->setEnabled(true);

	std::thread experimentThread(&MainWindow::RunFunc, this);
	experimentThread.detach();
}

void MainWindow::StopReceive() {
	receiver->Stop();
	startReceiveAction->setEnabled(true);
	stopReceiverAction->setEnabled(false);

	std::thread tryMergeThread([&]() {
		//reconstructor->TryMergeModels();
		//reconstructor->FinalGlobalBA();
		});
	tryMergeThread.detach();
}

// [Deprecated]
void MainWindow::ProcessImage(const std::string& newImagePath) {
	std::cout << "New image: " << newImagePath << std::endl;
	imageListWidget->AddImage(newImagePath);
}

void MainWindow::ExportProject() {
	std::thread exportProjectThread([&]() {
		const std::string basePath = StringReplace(EnsureTrailingSlash(GetParentDir(Database::exportPath)), "\\", "/");
		if (database) {
			cout << "Exporting database..." << endl;
			database->Export(Database::exportPath);
			cout << "Export database completed!" << endl;
		}
		if (reconstructionManager) {
			cout << "Exporting models..." << endl;
			for (int i = 0; i < reconstructionManager->Size(); i++) {
				const std::string modelExportDir = basePath + "models/" + to_string(i + 1);
				CreateDirIfNotExists(modelExportDir + "/bin", true);
				CreateDirIfNotExists(modelExportDir + "/text", true);
				reconstructionManager->Get(i)->WriteBinary(modelExportDir + "/bin");
				reconstructionManager->Get(i)->WriteText(modelExportDir + "/text");
				reconstructionManager->Get(i)->OutputDebugResult(modelExportDir);
			}
			cout << "Export mdoels completed!" << endl;
		}
		logWidget->SaveLogToFile(basePath + "Log.txt");

		MatchMatrixWidget::SaveMatchMatrix(basePath + "match_matrix.bmp", database);
		});
	exportProjectThread.detach();
}

void MainWindow::ExportRunningProject() {
	std::thread exportProjectThread([&]() {
		const std::string basePath = StringReplace(EnsureTrailingSlash(GetParentDir(Database::exportPath)), "\\", "/");
		if (database) {
			cout << "Exporting database..." << endl;
			database->Export(Database::exportPath);
			cout << "Export database completed!" << endl;
		}
		if (reconstructionManager) {
			cout << "Exporting models..." << endl;
			for (int i = 0; i < reconstructionManager->Size(); i++) {
				const std::string modelExportDir = basePath + "models/" + to_string(i + 1);
				CreateDirIfNotExists(modelExportDir + "/bin", true);
				CreateDirIfNotExists(modelExportDir + "/text", true);
				reconstructionManager->Get(i)->WriteBinary(modelExportDir + "/bin");
				reconstructionManager->Get(i)->WriteText(modelExportDir + "/text");
				reconstructionManager->Get(i)->OutputDebugResult(modelExportDir);
			}
			cout << "Export mdoels completed!" << endl;
		}
		logWidget->SaveLogToFile(basePath + "Log.txt");
		});
	exportProjectThread.detach();
}

void MainWindow::ExportModelOnRegisted() {
	auto& config = Config::getInstance();

	if (config.pipeline.AutoExportModel) {
		const size_t numRegImages = reconstructionManager->Get(0)->NumRegImages();
		std::thread exportProjectThread([&]() {
			const std::string basePath = StringReplace(EnsureTrailingSlash(GetParentDir(Database::exportPath)), "\\", "/");
			const std::string modelExportDir = basePath + "models/" + to_string(numRegImages);
			CreateDirIfNotExists(modelExportDir + "/bin", true);
			CreateDirIfNotExists(modelExportDir + "/text", true);
			reconstructionManager->Get(0)->WriteBinary(modelExportDir + "/bin");
			reconstructionManager->Get(0)->WriteText(modelExportDir + "/text");
			reconstructionManager->Get(0)->OutputDebugResult(modelExportDir);
			});
		exportProjectThread.detach();
	}
}

// ================= UI交互相关 =================
void MainWindow::ChangeCurrentModel() {
	/*if (isTrackModelCheckBox->isChecked()) {
		lock_guard<mutex> lock(renderMutex);
		const int lastRegModelID = Reconstructor::lastRegModelID;
		if (lastRegModelID >= 0 && lastRegModelID < reconstructionManager->Size()) {
			reconstructionManagerWidget->Update();
			reconstructionManagerWidget->SelectReconstruction(lastRegModelID);
			renderNowAction->trigger();
		}
	}
	else {
		reconstructionManagerWidget->Update();
		renderNowAction->trigger();
	}*/
}

void MainWindow::ShowMatchMatrix() {
	if (showMatchMatrixWidget) {
		showMatchMatrixWidget->show();
	}
}

void MainWindow::NewProject() {
	projectWidget->show();
}

void MainWindow::ChangeSelectModel() {
	isForcedRender = true;
	RenderNow();
	isForcedRender = false;
}

void MainWindow::RenderNow() {
#ifdef DISABLE_MODELVIEWER
	return;
#endif
	lock_guard<mutex> lock(renderMutex);
	try {
		if (reconstructionManager->Size() == 0) {
			reconstructionManagerWidget->SelectReconstruction(ReconstructionManagerWidget::kNewestReconstructionIdx);
			modelViewerWidget->ClearReconstruction();
			return;
		}
		reconstructionManagerWidget->Update();
		size_t reconstruction_idx = reconstructionManagerWidget->SelectedReconstructionIdx();
		if (reconstruction_idx == ReconstructionManagerWidget::kNewestReconstructionIdx) {
			if (reconstructionManager->Size() > 0) {
				reconstruction_idx = reconstructionManager->Size() - 1;
			}
		}
		/*if (isTrackModelCheckBox->isChecked() || reconstruction_idx == reconstructor->lastRegModelID || isForcedRender) {
			modelViewerWidget->reconstruction = reconstructionManager->Get(reconstruction_idx);
			modelViewerWidget->ReloadReconstruction();
		}*/
		if (isTrackModelCheckBox->isChecked() || isForcedRender) {
			modelViewerWidget->reconstruction = reconstructionManager->Get(reconstruction_idx);
			modelViewerWidget->ReloadReconstruction();
		}
	}
	catch (const std::exception& e) {
		std::cout << "RenderNow unknown exception caught" << endl;
	}
}

void MainWindow::NewProject_SLOT() {
	exportProjectAction->setEnabled(true);
}

void MainWindow::addNewImageInWidget(QString p_imagePath)
{
	std::string newImagePath = p_imagePath.toStdString();
	imageListWidget->AddImage(newImagePath);
}

// ================= 状态栏相关 =================

void MainWindow::onTabChanged(int index) {
	if (tabWidget->currentWidget() == modelViewerWidget) {
		statusLabel->setText(modelViewerWidget->status_label);
	}
}

