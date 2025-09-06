#include "TestMainWindow.h"
#include "../Base/config.h"
#include "../Scene/meshing.h"
#include <chrono>
class ManualTimer {
public:
    void Start() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    void Stop(const std::string& message = "") {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start_ );
        LOG(INFO) << message << " Took " << duration.count() << " milliseconds";
        Reset();
    }
    void Reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

TestMainWindow::TestMainWindow(QWidget* parent) :QMainWindow(parent)
{
    resize(1000 , 600);
    setWindowTitle(tr("OTF Feedback"));

    meshViewerWidget = new MeshViewerWidget(this);
    setCentralWidget(meshViewerWidget);

    QFont font;
    font.setPointSize(11);

    CreateWidgets();
    CreateActions();
    CreateToolbar();
}

void TestMainWindow::CreateWidgets() {
    log_widget_ = new LogWidget(this);
    dock_log_widget_ = new QDockWidget("Log" , this);
    dock_log_widget_->setWidget(log_widget_);
    addDockWidget(Qt::BottomDockWidgetArea , dock_log_widget_);
}

void TestMainWindow::CreateToolbar() {
    reconstruction_toolbar_ = addToolBar(tr("Reconstruction"));
    for (QAction* act : actions) {
        reconstruction_toolbar_->addAction(act);
    }
}

void TestMainWindow::CreateActions() {
    actions.push_back(new QAction("Create" , this));
    connect(actions[0] ,
        &QAction::triggered , this ,
        &TestMainWindow::CreateMesh);

    actions.push_back(new QAction("Update" , this));
    connect(actions[1] ,
        &QAction::triggered , this ,
        &TestMainWindow::UpdateMesh);

    actions.push_back(new QAction("Render" , this));
    connect(actions[2] ,
        &QAction::triggered , this ,
        &TestMainWindow::RenderMesh);
}

void TestMainWindow::CreateMesh()
{
    current_model_index = 1;
    std::thread t([&]() {
        LOG(INFO)<<"start create mesh";
        auto& config = Config::getInstance();
        string modelFolder = config.test.ModelFolder;
        int start = config.test.Start;
        int step = config.test.Step;
        int maxids = config.test.End;

        int modelindex = start;
        while (modelindex <= maxids) {
            fs::path fullPath = fs::path(modelFolder) / std::to_string(modelindex) / "text";
            subFolders.push_back(fullPath);
            modelindex += step;
        }
        reconstruction = std::make_shared<Reconstruction>();
        reconstruction->Read(subFolders.front().string());

        meshManager = new MeshManager();
        meshManager->CreateMesh(*reconstruction);
        meshManager->GenerateRenderMeshData(*reconstruction);
        actions[2]->trigger();
        });
    t.detach();
}

void TestMainWindow::UpdateMesh()
{
    std::thread t([&]() {
        ManualTimer t1;
        LOG(INFO) << "update mesh";
        reconstruction = std::make_shared<Reconstruction>();
        reconstruction->Read(subFolders[current_model_index].string());
        t1.Start();
        meshManager->UpdateMesh(*reconstruction);
        LOG(INFO) << "start calculate mesh data";
        meshManager->GenerateRenderMeshData(*reconstruction);
        t1.Stop("Update Time: ");
        current_model_index++;
        actions[2]->trigger();
        });
    t.detach();
}

void TestMainWindow::RenderMesh()
{
    meshViewerWidget->reconstruction = reconstruction;
    meshViewerWidget->mesh = meshManager->GetMeshPtr();
    meshViewerWidget->rendering_data = meshManager->GetRenderingData();
    meshViewerWidget->ReloadReconstructionWithFeedback(false , true , false);
}

