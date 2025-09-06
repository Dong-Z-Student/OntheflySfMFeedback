#include "../Base/macros.h"
#include "../Base/misc.h"
#include "../Feature/Common.h"
#include "../Feature/PyLoader.h"
#include "../Base/config.h"
#include "Workflow.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
//#include <boost/stacktrace.hpp>


using namespace std;

ImageReader CWorkflow::imageReader;
thread_local CSIFTCPUExtractor* CWorkflow::SIFTCPUExtractor = nullptr;
thread_local CSIFTGPUExtractor* CWorkflow::SIFTGPUExtractor = nullptr;
thread_local CSIFTCPUMatcher* CWorkflow::SIFTCPUMatcher = nullptr;
thread_local CSIFTGPUMatcher* CWorkflow::SIFTGPUMatcher = nullptr;
PyLoader* CWorkflow::pyloader = nullptr;
CGlobalFeatureExtractor* CWorkflow::globalFeatureExtractor = nullptr;
CPythonExtractor* CWorkflow::PYTHONExtractor = nullptr;
CPythonMatcher* CWorkflow::PYTHONMatcher = nullptr;
Reconstructor* CWorkflow::reconstructor = nullptr;
CGlobalFeatureRetriever* CWorkflow::globalFeatureRetriever = nullptr;

mutex CWorkflow::outputMutex;
Database* CWorkflow::database = nullptr;
//COptions* CWorkflow::options = nullptr;

size_t CWorkflow::numReadThreads = 1;
size_t CWorkflow::numGPUThreads = 1;
size_t CWorkflow::numSIFTExtractCPUThreads = 2;
size_t CWorkflow::numSIFTMatchCPUThreads = 4;
size_t CWorkflow::numEstimateThreads = 8;

vector<thread> CWorkflow::readThreads;
vector<thread> CWorkflow::GPUThreads;
vector<thread> CWorkflow::SIFTExtractCPUThreads;
vector<thread> CWorkflow::SIFTMatchCPUThreads;
vector<thread> CWorkflow::estimateThreads;
thread CWorkflow::retrievalThread;
thread CWorkflow::reconstructThread;
thread CWorkflow::writeRegThread;
thread CWorkflow::pythonThread;
thread CWorkflow::feedbackThread;

atomic_bool CWorkflow::isStop = false;
size_t CWorkflow::reconstructNextImageIndex = 2; // 因为第1号影像不用重建, 从2开始

unordered_map<size_t , string> CWorkflow::allImages;
unordered_map<string , size_t> CWorkflow::allImagePaths;

atomic_bool CWorkflow::isBusy = false;
size_t CWorkflow::retrievalTopN = 30;

tbb::concurrent_map<std::size_t , std::atomic_size_t> CWorkflow::SIFTExtractionTimes;
tbb::concurrent_map<std::size_t , std::atomic_size_t> CWorkflow::globalFeatureExtractionTimes;
tbb::concurrent_map<std::size_t , std::atomic_size_t> CWorkflow::globalFeatureRetrievalTimes;
tbb::concurrent_map<std::size_t , std::atomic_size_t> CWorkflow::SIFTMatchingTimes;
tbb::concurrent_map<std::size_t , std::atomic_size_t> CWorkflow::geometricVerificationTimes;



#ifdef _DEBUG
queue<string> CWorkflow::readTasks;
queue<std::pair<Bitmap , std::string>> CWorkflow::SIFTExtractCPUTasks;
queue<std::pair<Bitmap , std::string>> CWorkflow::SIFTExtractGPUTasks;
queue<pair<size_t , size_t>> CWorkflow::SIFTMatchCPUTasks;
queue<pair<size_t , size_t>> CWorkflow::SIFTMatchGPUTasks;
queue<pair<size_t , size_t>> CWorkflow::estimateTasks;
unordered_set<size_t> CWorkflow::reconstructionTasks;
unordered_map<string , CImageStatusFlag> CWorkflow::imagesStatus;
unordered_map<size_t , unordered_set<size_t>> CWorkflow::matchingPairs;


mutex CWorkflow::readTasks_Mu;
mutex CWorkflow::SIFTExtractCPUTasks_Mu;
mutex CWorkflow::SIFTExtractGPUTasks_Mu;
mutex CWorkflow::SIFTMatchCPUTasks_Mu;
mutex CWorkflow::SIFTMatchGPUTasks_Mu;
mutex CWorkflow::estimateTasks_Mu;
mutex CWorkflow::imagesStatus_Mu;
mutex CWorkflow::matchingPairs_Mu;
mutex CWorkflow::reconstructionTasks_Mu;
#else

tbb::concurrent_queue<std::string> CWorkflow::readTasks;
tbb::concurrent_queue<std::pair<Bitmap , std::string>> CWorkflow::SIFTExtractCPUTasks;
tbb::concurrent_queue<std::pair<Bitmap , std::string>> CWorkflow::SIFTExtractGPUTasks;
tbb::concurrent_queue<std::string> CWorkflow::globalFeatureExtractionTasks;
tbb::concurrent_queue<pair<size_t , size_t>> CWorkflow::SIFTMatchCPUTasks;
tbb::concurrent_queue<pair<size_t , size_t>> CWorkflow::SIFTMatchGPUTasks;
tbb::concurrent_queue<pair<size_t , size_t>> CWorkflow::estimateTasks;
tbb::concurrent_unordered_map<string , CImageStatusFlag> CWorkflow::imagesStatus;
tbb::concurrent_unordered_map<size_t , atomic_size_t> CWorkflow::numMatchingPairs;
tbb::concurrent_unordered_set<std::size_t> CWorkflow::reconstructionTasks;
tbb::concurrent_queue<std::size_t> CWorkflow::retrievalTasks;

#endif


void ScaleKeypoints(const Bitmap& bitmap , const Camera& camera , FeatureKeypoints* keypoints) {
    if (static_cast<size_t>( bitmap.Width() ) != camera.Width() ||
        static_cast<size_t>( bitmap.Height() ) != camera.Height()) {
        const float scale_x = static_cast<float>( camera.Width() ) / bitmap.Width();
        const float scale_y = static_cast<float>( camera.Height() ) / bitmap.Height();
        for (auto& keypoint : *keypoints) {
            keypoint.Rescale(scale_x , scale_y);
        }
    }
}



CWorkflow::~CWorkflow()
{
    isStop = true;
#ifndef _DEBUG
    readTasks.clear();
    SIFTExtractCPUTasks.clear();
    SIFTExtractGPUTasks.clear();
    SIFTMatchCPUTasks.clear();
    SIFTMatchGPUTasks.clear();
    estimateTasks.clear();
#endif
}
void CWorkflow::Initialize(Database* database , Reconstructor* reconstructor , size_t numReadThreads , size_t numGPUThreads , size_t numSIFTExtractCPUThreads , size_t numSIFTMatchCPUThreads , size_t numEstimateThreads)
{
    CHECK(database);
    SetDatabase(database);

    CWorkflow::reconstructor = reconstructor;
    CWorkflow::numReadThreads = numReadThreads;
    CWorkflow::numGPUThreads = numGPUThreads;
    CWorkflow::numSIFTExtractCPUThreads = numSIFTExtractCPUThreads;
    CWorkflow::numSIFTMatchCPUThreads = numSIFTMatchCPUThreads;
    CWorkflow::numEstimateThreads = numEstimateThreads;

    readThreads.resize(CWorkflow::numReadThreads);
    for (size_t i = 0; i < CWorkflow::numReadThreads; i++)
    {
        readThreads[i] = thread(Read);
        readThreads[i].detach();
    }

    GPUThreads.resize(CWorkflow::numGPUThreads);
    for (size_t i = 0; i < CWorkflow::numGPUThreads; i++)
    {
        GPUThreads[i] = thread(PyGPU);
        GPUThreads[i].detach();
    }

    SIFTExtractCPUThreads.resize(CWorkflow::numSIFTExtractCPUThreads);
    for (size_t i = 0; i < CWorkflow::numSIFTExtractCPUThreads; i++)
    {
        SIFTExtractCPUThreads[i] = thread(SIFTExtractCPU);
        SIFTExtractCPUThreads[i].detach();
    }

    SIFTMatchCPUThreads.resize(CWorkflow::numSIFTMatchCPUThreads);
    for (size_t i = 0; i < CWorkflow::numSIFTMatchCPUThreads; i++)
    {
        SIFTMatchCPUThreads[i] = thread(SIFTMatchCPU);
        SIFTMatchCPUThreads[i].detach();
    }

    estimateThreads.resize(CWorkflow::numEstimateThreads);
    for (size_t i = 0; i < CWorkflow::numEstimateThreads; i++)
    {
        estimateThreads[i] = thread(Estimate);
        estimateThreads[i].detach();
    }

    retrievalThread = thread(Retrieval);
    retrievalThread.detach();

    reconstructThread = thread(Reconstruct);
    reconstructThread.detach();

    writeRegThread = thread(WriteReg);
    writeRegThread.detach();

    pythonThread = thread(LoadPython);
    pythonThread.detach();

    feedbackThread = thread(MeshingModel);
    feedbackThread.detach();

    WriteReconstructStatus(true);
}
void CWorkflow::SetDatabase(Database* database)
{
    CWorkflow::database = database;
}
void CWorkflow::Stop()
{
    isStop = true;
}
void CWorkflow::AddImageProcess(const string& imagePath)
{
#ifdef _DEBUG
    scoped_lock lock(imagesStatus_Mu);
#endif

    readTasks.push(imagePath);
    imagesStatus[imagePath] = CImageStatusFlag::CUnread;
}
void CWorkflow::LoadPython()
{
    this_thread::sleep_for(chrono::milliseconds(1000));
    pyloader = new PyLoader();
    PYTHONExtractor = new CPythonExtractor();
    PYTHONMatcher = new CPythonMatcher();
#ifndef NOT_USE_GLOBAL_FEATURES
    globalFeatureExtractor = new CGlobalFeatureExtractor();
    globalFeatureRetriever = new CGlobalFeatureRetriever(database , retrievalTopN);
#endif
}
bool CWorkflow::SIFTExtract(const std::string& imagePath , const CSIFTExtractionOptions& options , Database& database)
{
    auto start = chrono::high_resolution_clock::now();
    if (!ExistsFile(imagePath))
    {
        std::cout << ( boost::format("[Read image] The path %1% dose not exist!") % imagePath ).str() << std::endl;
        //CLog::Log((boost::format("[Read image] The path %1% dose not exist!") % imagePath).str());
        return false;
    }
    Camera camera;
    std::string cameraModel;
    Bitmap bitmap;
    Image image;
    if (imageReader.Read(imagePath , camera , cameraModel , image , bitmap) == ImageReader::Status::SUCCESS)
    {
        const camera_t cameraID = database.WriteCamera(camera);
        camera.SetCameraId(cameraID);
        image.SetCameraId(cameraID);
        const image_t imageID = database.WriteImage(image);

        FeatureKeypoints keypoints;
        FeatureDescriptors descriptors;

        bool isSuccess = false;
        if (options.isUseGPU)
        {
            if (!SIFTGPUExtractor)
            {
                SIFTGPUExtractor = new CSIFTGPUExtractor(options);
            }
            isSuccess = SIFTGPUExtractor->Extract(bitmap , keypoints , descriptors);
        }
        if (!isSuccess)
        {
            if (!SIFTCPUExtractor)
            {
                SIFTCPUExtractor = new CSIFTCPUExtractor(options);
            }
            keypoints.clear();
            isSuccess = SIFTCPUExtractor->Extract(bitmap , keypoints , descriptors);
        }
        if (!isSuccess)
        {
            std::cout << ( boost::format("[Extraction] Error occurred while extracting SIFT features from image %1% !") % imagePath ).str() << std::endl;
            return false;
        }

        ScaleKeypoints(bitmap , camera , &keypoints);

        database.WriteKeypoints(imageID , keypoints);
        database.WriteDescriptors(imageID , descriptors);

        auto end = chrono::high_resolution_clock::now();
        size_t timeConsuming = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
        std::cout << ( boost::format("[Read image %1%ms] Image %2% has been successfully extracted!") % to_string(timeConsuming) % imagePath ).str() << std::endl;
        return true;
    }

    std::cout << "Read error!" << std::endl;
    return false;
}
bool CWorkflow::SIFTMatch(Database& database , size_t imageID1 , size_t imageID2 , const CSIFTMatchingOptions& options)
{
    if (!database.ExistsImage(imageID1) || !database.ExistsImage(imageID2))
    {
        return false;
    }
    if (database.NumDescriptorsForImage(imageID1) == 0 || database.NumDescriptorsForImage(imageID2) == 0)
    {
        return false;
    }

    const FeatureDescriptors& descriptors1 = database.ReadDescriptors(imageID1);
    const FeatureDescriptors& descriptors2 = database.ReadDescriptors(imageID2);
    CHECK(descriptors1.cols() == 128 && descriptors2.cols() == 128);

    FeatureMatches matches;
    if (options.isUseGPU)
    {
        if (!SIFTGPUMatcher)
        {
            SIFTGPUMatcher = new CSIFTGPUMatcher(options);
        }

        CSIFTGPUMatcher::isUploadDescriptors1 = ( CSIFTGPUMatcher::lastDescriptors1Index != imageID1 );
        CSIFTGPUMatcher::isUploadDescriptors2 = ( CSIFTGPUMatcher::lastDescriptors2Index != imageID2 );
        matches = SIFTGPUMatcher->Match(descriptors1 , descriptors2);
        CSIFTGPUMatcher::lastDescriptors1Index = imageID1;
        CSIFTGPUMatcher::lastDescriptors2Index = imageID2;
    }
    else
    {
        if (!SIFTCPUMatcher)
        {
            SIFTCPUMatcher = new CSIFTCPUMatcher(options);
        }
        matches = SIFTCPUMatcher->Match(descriptors1 , descriptors2);
    }
    if (!matches.empty())
    {
        database.WriteMatches(imageID1 , imageID2 , matches);
    }
    return true;
}

bool CWorkflow::EstimateTwoViewGeometry(Database& database , size_t imageID1 , size_t imageID2)
{
    if (!database.ExistsImage(imageID1) || !database.ExistsImage(imageID2))
    {
        return false;
    }

    const FeatureKeypoints keypoints1 = database.ReadKeypoints(imageID1);
    const FeatureKeypoints keypoints2 = database.ReadKeypoints(imageID2);
    if (keypoints1.empty() || keypoints2.empty() || !database.ExistsMatches(imageID1 , imageID2))
    {
        return true;
    }
    const vector<Eigen::Vector2d> pointsVector1 = FeatureKeypointsToPointsVector(keypoints1);
    const vector<Eigen::Vector2d> pointsVector2 = FeatureKeypointsToPointsVector(keypoints2);
    const FeatureMatches matches = database.ReadMatches(imageID1 , imageID2);
    if (matches.empty())
    {
        return true;
    }

    const Camera camera1 = database.ReadCamera(database.ReadImage(imageID1).CameraId());
    const Camera camera2 = database.ReadCamera(database.ReadImage(imageID2).CameraId());

    // 如果有相机的焦距为0, 会导致双视几何估计的过程存在死循环
    if (abs(camera1.FocalLength()) < 1e-5 || abs(camera2.FocalLength()) < 1e-5)
    {
        return false;
    }

    TwoViewGeometryOptions options;
    const TwoViewGeometry twoViewGeometry = EstimateTwoViewGeometry_Internal(camera1 , pointsVector1 , camera2 , pointsVector2 , matches , options);
    if (!twoViewGeometry.inlier_matches.empty())
    {
        database.WriteTwoViewGeometry(imageID1 , imageID2 , twoViewGeometry);
    }
    return true;
}
void CWorkflow::OutputTimeFiles(const std::string& outputDir)
{
    const std::string baseDir = StringReplace(EnsureTrailingSlash(outputDir) , "\\" , "/");

    std::ofstream SIFTExtractionTimeFile(baseDir + "SIFTExtractionTimes.txt");
    for (const auto& pair : SIFTExtractionTimes)
    {
        const size_t imageID = pair.first;
        const size_t SIFTExtractionTime = pair.second;
        SIFTExtractionTimeFile << imageID << "," << SIFTExtractionTime << std::endl;
    }
    SIFTExtractionTimeFile.close();

    std::ofstream globalFeatureExtractionTimeFile(baseDir + "globalFeatureExtractionTimes.txt");
    for (const auto& pair : globalFeatureExtractionTimes)
    {
        const size_t imageID = pair.first;
        const size_t globalFeatureExtractionTime = pair.second;
        globalFeatureExtractionTimeFile << imageID << "," << globalFeatureExtractionTime << std::endl;
    }
    globalFeatureExtractionTimeFile.close();

    std::ofstream globalFeatureRetrievalTimeFile(baseDir + "globalFeatureRetrievalTimes.txt");
    for (const auto& pair : globalFeatureRetrievalTimes)
    {
        const size_t imageID = pair.first;
        const size_t globalFeatureRetrievalTime = pair.second;
        globalFeatureRetrievalTimeFile << imageID << "," << globalFeatureRetrievalTime << std::endl;
    }
    globalFeatureRetrievalTimeFile.close();

    std::ofstream SIFTMatchingTimeFile(baseDir + "SIFTMatchingTimes.txt");
    for (const auto& pair : SIFTMatchingTimes)
    {
        const size_t imageID = pair.first;
        const size_t SIFTMatchingTime = pair.second;
        SIFTMatchingTimeFile << imageID << "," << SIFTMatchingTime << std::endl;
    }
    SIFTMatchingTimeFile.close();

    std::ofstream geometricVerificationTimeFile(baseDir + "geometricVerificationTimes.txt");
    for (const auto& pair : geometricVerificationTimes)
    {
        const size_t imageID = pair.first;
        const size_t geometricVerificationTime = pair.second;
        geometricVerificationTimeFile << imageID << "," << geometricVerificationTime << std::endl;
    }
    geometricVerificationTimeFile.close();
}

void CWorkflow::Read()
{
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        if (!database)
        {
            continue;
        }

#ifdef _DEBUG
        scoped_lock lock(readTasks_Mu , imagesStatus_Mu);
        if (readTasks.empty())
        {
            continue;
        }
        const string imagePath = readTasks.front();
        readTasks.pop();

        CHECK(imagesStatus.at(imagePath) == CImageStatusFlag::CUnread);
        if (!ExistsFile(imagePath))
        {
            std::cout << ( boost::format("[Read image] The path %1% dose not exist!") % imagePath ).str() << std::endl;
            //CLog::Log((boost::format("[Read image] The path %1% dose not exist!") % imagePath).str());
            continue;
        }
        Camera camera;
        std::string cameraModel;
        Bitmap bitmap;
        Image image;
        if (imageReader.Read(imagePath , camera , cameraModel , image , bitmap) == ImageReader::Status::SUCCESS)
        {
            const camera_t cameraID = database->WriteCamera(camera);
            camera.SetCameraId(cameraID);
            image.SetCameraId(cameraID);
            const image_t imageID = database->WriteImage(image);

            allImages[imageID] = imagePath;
            allImagePaths[imagePath] = imageID;

            lock_guard<mutex> SIFTExtractGPULock(SIFTExtractGPUTasks_Mu);
            SIFTExtractGPUTasks.push(make_pair(bitmap , imagePath));
        }
        imagesStatus.at(imagePath) = CImageStatusFlag::CRead;

#else
        if (readTasks.empty())
        {
            continue;
        }
        string imagePath = "";
        if (!readTasks.try_pop(imagePath) || imagePath.empty())
        {
            continue;
        }
        if (!ExistsFile(imagePath))
        {
            std::cout << ( boost::format("[Read image] The path %1% dose not exist!") % imagePath ).str() << std::endl;
            continue;
        }
        Camera camera;
        std::string cameraModel;
        Bitmap bitmap;
        Image image;

        Timer imageReaderTimer;
        imageReaderTimer.Start();

        if (imageReader.Read(imagePath , camera , cameraModel , image , bitmap) == ImageReader::Status::SUCCESS)
        {
            const camera_t cameraID = database->WriteCamera(camera);
            camera.SetCameraId(cameraID);
            image.SetCameraId(cameraID);
            const image_t imageID = database->WriteImage(image);

            allImages[imageID] = imagePath;
            allImagePaths[imagePath] = imageID;

            const size_t readTime = imageReaderTimer.ElapsedMicroSeconds() / 1000;
            SIFTExtractionTimes[imageID] = readTime;
            globalFeatureExtractionTimes[imageID] = 0;
            globalFeatureRetrievalTimes[imageID] = 0;
            SIFTMatchingTimes[imageID] = 0;
            geometricVerificationTimes[imageID] = 0;

            SIFTExtractGPUTasks.push(make_pair(bitmap , imagePath));
#ifndef NOT_USE_GLOBAL_FEATURES
            globalFeatureExtractionTasks.push(imagePath);
#endif

        }
        imagesStatus.at(imagePath) = CImageStatusFlag::CRead;
#endif
    }
}
[[deprecated("请使用 PyGPU 替代 GPU，用于支持基于深度学习的特征提取匹配方法")]]
void CWorkflow::GPU()
{
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(50));
        if (!database)
        {
            continue;
        }

#ifdef _DEBUG
        {
            scoped_lock lock(SIFTExtractGPUTasks_Mu , imagesStatus_Mu);
            if (!SIFTExtractGPUTasks.empty())
            {
                const pair<Bitmap , string> info = SIFTExtractGPUTasks.front();
                SIFTExtractGPUTasks.pop();
                CHECK(imagesStatus.at(info.second) == CImageStatusFlag::CRead);
                imagesStatus.at(info.second) = CImageStatusFlag::CExtracting;

                if (!SIFTGPUExtractor)
                {
                    CSIFTExtractionOptions options;
                    SIFTGPUExtractor = new CSIFTGPUExtractor(options);
                }
                FeatureKeypoints keypoints;
                FeatureDescriptors descriptors;

                if (SIFTGPUExtractor->Extract(info.first , keypoints , descriptors))
                {
                    const size_t imageID = allImagePaths.at(info.second);
                    ScaleKeypoints(info.first , database->ReadCamera(database->ReadImage(imageID).CameraId()) , &keypoints);


                    database->WriteKeypoints(imageID , keypoints);
                    database->WriteDescriptors(imageID , descriptors);
                    imagesStatus.at(info.second) = CImageStatusFlag::CExtracted;

                    std::cout << ( boost::format("[Extraction] Image %1% extracted successed!") % info.second ).str() << std::endl;
                    scoped_lock lock2(SIFTMatchGPUTasks_Mu , matchingPairs_Mu);
                    for (size_t matchedImageID = 1; matchedImageID < imageID; matchedImageID++)
                    {
                        SIFTMatchGPUTasks.push({ matchedImageID, imageID });
                        matchingPairs[imageID].insert(matchedImageID);
                    }
                    if (matchingPairs.find(imageID) != matchingPairs.end() && !matchingPairs.at(imageID).empty())
                    {
                        imagesStatus.at(info.second) = CImageStatusFlag::CMatching;
                    }
                    else
                    {
                        imagesStatus.at(info.second) = CImageStatusFlag::CMatched;
                    }
                }
                else
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatched;
                }
            }
        }
        {
            scoped_lock lock(SIFTMatchGPUTasks_Mu , imagesStatus_Mu);
            if (!SIFTMatchGPUTasks.empty())
            {
                const pair<size_t , size_t> matchingPair = SIFTMatchGPUTasks.front();
                SIFTMatchGPUTasks.pop();

                CSIFTMatchingOptions options;
                if (CWorkflow::SIFTMatch(*database , matchingPair.first , matchingPair.second , options))
                {
                    const size_t numMatches = ( database->ExistsMatches(matchingPair.first , matchingPair.second) ? database->ReadMatches(matchingPair.first , matchingPair.second).size() : 0 );
                    std::cout << ( boost::format("[MatchGPU] %1% - %2%: %3%") % matchingPair.first % matchingPair.second % numMatches ).str() << std::endl;

                    lock_guard<mutex> estimateLock(estimateTasks_Mu);
                    estimateTasks.push(matchingPair);
                }
            }

        }
#else
        if (!SIFTExtractGPUTasks.empty())
        {
            pair<Bitmap , string> info;
            if (!SIFTExtractGPUTasks.try_pop(info))
            {
                continue;
            }
            const std::string imagePath = info.second;

            CHECK(imagesStatus.at(imagePath) == CImageStatusFlag::CRead);
            imagesStatus.at(imagePath) = CImageStatusFlag::CExtracting;

            Timer SIFTExtractionTimer;
            SIFTExtractionTimer.Start();

            if (!SIFTGPUExtractor)
            {
                CSIFTExtractionOptions options;
                SIFTGPUExtractor = new CSIFTGPUExtractor(options);
            }
            FeatureKeypoints keypoints;
            FeatureDescriptors descriptors;
            if (SIFTGPUExtractor->Extract(info.first , keypoints , descriptors))
            {
                const size_t imageID = allImagePaths.at(imagePath);
                ScaleKeypoints(info.first , database->ReadCamera(database->ReadImage(imageID).CameraId()) , &keypoints);
                database->WriteKeypoints(imageID , keypoints);
                database->WriteDescriptors(imageID , descriptors);
                imagesStatus.at(imagePath) = CImageStatusFlag::CExtracted;

                const size_t SIFTExtractionTime = SIFTExtractionTimer.ElapsedMicroSeconds() / 1000;
                SIFTExtractionTimes.at(imageID) += SIFTExtractionTime;

                std::cout << ( boost::format("[Extraction] Image %1% extracted successed!") % imagePath ).str() << std::endl;

#ifdef NOT_USE_GLOBAL_FEATURES
                if (imageID > 1)
                {
                    numMatchingPairs[imageID] = imageID - 1;
                }
                for (size_t matchedImageID = 1; matchedImageID < imageID; matchedImageID++)
                {
                    SIFTMatchGPUTasks.push({ matchedImageID, imageID });
                }

                if (numMatchingPairs.find(imageID) != numMatchingPairs.end() && numMatchingPairs.at(imageID) > 0)
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatching;
                }
                else
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatched;
                }
#endif
            }
            continue;
        }
#ifndef NOT_USE_GLOBAL_FEATURES
        if (!globalFeatureExtractionTasks.empty())
        {
            std::string imagePath;
            if (!globalFeatureExtractionTasks.try_pop(imagePath))
            {
                continue;
            }


            Timer globalFeatureExtractionTimer;
            globalFeatureExtractionTimer.Start();

            GlobalFeature globalFeature;
            if (globalFeatureExtractor->Extract(imagePath , globalFeature))
            {
                CHECK(!globalFeature.empty());
                const size_t imageID = allImagePaths.at(imagePath);
                database->WriteGlobalFeature(imageID , globalFeature);
                retrievalTasks.push(imageID);

                const size_t globalFeatureExtractionTime = globalFeatureExtractionTimer.ElapsedMicroSeconds() / 1000;
                globalFeatureExtractionTimes.at(imageID) = globalFeatureExtractionTime;

                std::cout << ( boost::format("[Global feature] Image %1% extracted successed!") % imagePath ).str() << std::endl;
            }
            else
            {
                std::cout << ( boost::format("[Global feature] Image %1% extracted failed!") % imagePath ).str() << std::endl;
            }
        }
#endif
        if (!SIFTMatchGPUTasks.empty())
        {
            constexpr size_t invalidIndex = numeric_limits<size_t>::max();
            pair<size_t , size_t> matchingPair = { invalidIndex, invalidIndex };
            if (!SIFTMatchGPUTasks.try_pop(matchingPair) || matchingPair.first == invalidIndex || matchingPair.second == invalidIndex)
            {
                continue;
            }

            Timer SIFTMatchingTimer;
            SIFTMatchingTimer.Start();

            CSIFTMatchingOptions options;
            if (CWorkflow::SIFTMatch(*database , matchingPair.first , matchingPair.second , options))
            {
                const size_t numMatches = ( database->ExistsMatches(matchingPair.first , matchingPair.second) ? database->ReadMatches(matchingPair.first , matchingPair.second).size() : 0 );
#ifndef NOT_USE_GLOBAL_FEATURES
                //std::cout << (boost::format("[MatchGPU] %1% - %2%: %3%") % matchingPair.first % matchingPair.second % numMatches).str() << std::endl;
#endif

                const size_t SIFTMatchingTime = SIFTMatchingTimer.ElapsedMicroSeconds() / 1000;
                SIFTMatchingTimes.at(matchingPair.second) += SIFTMatchingTime;

                estimateTasks.push(matchingPair);
            }
            else
            {
                std::cout << "SIFTMatchGPUTasks warning!" << std::endl;
                reconstructionTasks.insert(matchingPair.second);
            }
        }
#endif
    }
}

void CWorkflow::PyGPU() {
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(50));
        if (!database)
        {
            continue;
        }
        if (!SIFTExtractGPUTasks.empty())
        {
            pair<Bitmap , string> info;
            if (!SIFTExtractGPUTasks.try_pop(info))
            {
                continue;
            }
            const std::string imagePath = info.second;

            CHECK(imagesStatus.at(imagePath) == CImageStatusFlag::CRead);
            imagesStatus.at(imagePath) = CImageStatusFlag::CExtracting;

            Timer ExtractionTimer;
            ExtractionTimer.Start();

#ifndef Superpoint_Feature    //使用原生GPU-SIFT
            if (!SIFTGPUExtractor)
            {
                CSIFTExtractionOptions options;
                SIFTGPUExtractor = new CSIFTGPUExtractor(options);
            }
            FeatureKeypoints keypoints;
            FeatureDescriptors descriptors;
            LOG(INFO) << "run sift extract";
            if (SIFTGPUExtractor->Extract(info.first , keypoints , descriptors))
            {
                const size_t imageID = allImagePaths.at(imagePath);
                ScaleKeypoints(info.first , database->ReadCamera(database->ReadImage(imageID).CameraId()) , &keypoints);
                database->WriteKeypoints(imageID , keypoints);
                database->WriteDescriptors(imageID , descriptors);
                imagesStatus.at(imagePath) = CImageStatusFlag::CExtracted;

                const size_t ExtractionTime = ExtractionTimer.ElapsedMicroSeconds() / 1000;
                SIFTExtractionTimes.at(imageID) += ExtractionTime;

                std::cout << ( boost::format("[Extraction] Image %1% extracted successed!") % imagePath ).str() << std::endl;



#ifdef NOT_USE_GLOBAL_FEATURES
                if (imageID > 1)
                {
                    numMatchingPairs[imageID] = imageID - 1;
                }
                for (size_t matchedImageID = 1; matchedImageID < imageID; matchedImageID++)
                {
                    SIFTMatchGPUTasks.push({ matchedImageID, imageID });
                }

                if (numMatchingPairs.find(imageID) != numMatchingPairs.end() && numMatchingPairs.at(imageID) > 0)
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatching;
                }
                else
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatched;
                }
#endif
            }
#endif

#ifdef Superpoint_Feature    //使用Superpoint提取
            PythonThreadLocker lock;
            FeatureKeypoints keypoints;
            FeatureDescriptors descriptors;
            logger.log("run superpoint extract");
            PyObject* featdict = PYTHONExtractor->Extract(info.second , keypoints , descriptors);
            if (featdict)
            {
                const size_t imageID = allImagePaths.at(imagePath);
                ScaleKeypoints(info.first , database->ReadCamera(database->ReadImage(imageID).CameraId()) , &keypoints);
                database->WriteKeypoints(imageID , keypoints);
                database->WriteDescriptors(imageID , descriptors);
                imagesStatus.at(imagePath) = CImageStatusFlag::CExtracted;

                const size_t ExtractionTime = ExtractionTimer.ElapsedMicroSeconds() / 1000;
                SIFTExtractionTimes.at(imageID) += ExtractionTime;

                std::cout << ( boost::format("[Extraction] Image %1% extracted successed!") % imagePath ).str() << std::endl;

                PYTHONExtractor->SaveFeatures(to_string(imageID) , featdict);

#ifdef NOT_USE_GLOBAL_FEATURES
                if (imageID > 1)
                {
                    numMatchingPairs[imageID] = imageID - 1;
                }
                for (size_t matchedImageID = 1; matchedImageID < imageID; matchedImageID++)
                {
                    SIFTMatchGPUTasks.push({ matchedImageID, imageID });
                }

                if (numMatchingPairs.find(imageID) != numMatchingPairs.end() && numMatchingPairs.at(imageID) > 0)
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatching;
                }
                else
                {
                    imagesStatus.at(info.second) = CImageStatusFlag::CMatched;
                }
#endif
            }
#endif
            continue;
        }

#ifndef NOT_USE_GLOBAL_FEATURES
        if (!globalFeatureExtractionTasks.empty())
        {
            PythonThreadLocker lock;
            std::string imagePath;
            if (!globalFeatureExtractionTasks.try_pop(imagePath))
            {
                continue;
            }

            Timer globalFeatureExtractionTimer;
            globalFeatureExtractionTimer.Start();

            GlobalFeature globalFeature;
            bool extractFlag = globalFeatureExtractor->Extract(imagePath , globalFeature);
            if (extractFlag)
            {
                CHECK(!globalFeature.empty());
                const size_t imageID = allImagePaths.at(imagePath);
                database->WriteGlobalFeature(imageID , globalFeature);
                retrievalTasks.push(imageID);

                const size_t globalFeatureExtractionTime = globalFeatureExtractionTimer.ElapsedMicroSeconds() / 1000;
                globalFeatureExtractionTimes.at(imageID) = globalFeatureExtractionTime;

                std::cout << ( boost::format("[Global feature] Image %1% extracted successed!") % imagePath ).str() << std::endl;
            }
            else
            {
                std::cout << ( boost::format("[Global feature] Image %1% extracted failed!") % imagePath ).str() << std::endl;
            }
        }
#endif
        if (!SIFTMatchGPUTasks.empty())
        {
            constexpr size_t invalidIndex = numeric_limits<size_t>::max();
            pair<size_t , size_t> matchingPair = { invalidIndex, invalidIndex };
            if (!SIFTMatchGPUTasks.try_pop(matchingPair) || matchingPair.first == invalidIndex || matchingPair.second == invalidIndex)
            {
                continue;
            }

            Timer SIFTMatchingTimer;
            SIFTMatchingTimer.Start();

#ifndef Lightglue_Match
            CSIFTMatchingOptions options;
            if (CWorkflow::SIFTMatch(*database , matchingPair.first , matchingPair.second , options))
            {
                const size_t numMatches = ( database->ExistsMatches(matchingPair.first , matchingPair.second) ? database->ReadMatches(matchingPair.first , matchingPair.second).size() : 0 );
#ifndef NOT_USE_GLOBAL_FEATURES
                //std::cout << (boost::format("[MatchGPU] %1% - %2%: %3%") % matchingPair.first % matchingPair.second % numMatches).str() << std::endl;
#endif

                const size_t SIFTMatchingTime = SIFTMatchingTimer.ElapsedMicroSeconds() / 1000;
                SIFTMatchingTimes.at(matchingPair.second) += SIFTMatchingTime;

                estimateTasks.push(matchingPair);
            }
            else
            {
                std::cout << "SIFTMatchGPUTasks warning!" << std::endl;
                reconstructionTasks.insert(matchingPair.second);
            }
#endif
#ifdef Lightglue_Match
            PythonThreadLocker lock;
            if (CWorkflow::PyMatch(*database , matchingPair.first , matchingPair.second)) {
                const size_t numMatches = ( database->ExistsMatches(matchingPair.first , matchingPair.second) ? database->ReadMatches(matchingPair.first , matchingPair.second).size() : 0 );
#ifndef NOT_USE_GLOBAL_FEATURES
                //std::cout << (boost::format("[MatchGPU] %1% - %2%: %3%") % matchingPair.first % matchingPair.second % numMatches).str() << std::endl;
#endif

                const size_t SIFTMatchingTime = SIFTMatchingTimer.ElapsedMicroSeconds() / 1000;
                SIFTMatchingTimes.at(matchingPair.second) += SIFTMatchingTime;

                estimateTasks.push(matchingPair);
            }
            else
            {
                std::cout << "SIFTMatchGPUTasks warning!" << std::endl;
                reconstructionTasks.insert(matchingPair.second);
            }
#endif
        }
    }
}

[[deprecated("已集成到PyGPU中")]]
bool CWorkflow::PyMatch(Database& database , size_t imageID1 , size_t imageID2)
{
    if (!database.ExistsImage(imageID1) || !database.ExistsImage(imageID2))
    {
        return false;
    }
    if (database.NumDescriptorsForImage(imageID1) == 0 || database.NumDescriptorsForImage(imageID2) == 0)
    {
        return false;
    }

    FeatureMatches matches;
    matches = PYTHONMatcher->Match(imageID1 , imageID2);



    if (!matches.empty())
    {
        database.WriteMatches(imageID1 , imageID2 , matches);
    }
    return true;
}
[[deprecated("已集成到PyGPU中")]]
void CWorkflow::SIFTExtractCPU()
{

}
[[deprecated("已集成到PyGPU中")]]
void CWorkflow::SIFTMatchCPU()
{

}

void CWorkflow::Estimate()
{
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        if (!database)
        {
            continue;
        }
#ifdef _DEBUG
        scoped_lock lock(estimateTasks_Mu , imagesStatus_Mu);
        if (estimateTasks.empty())
        {
            continue;
        }

        const pair<size_t , size_t> estimatePair = estimateTasks.front();
        estimateTasks.pop();
        if (CWorkflow::EstimateTwoViewGeometry(*database , estimatePair.first , estimatePair.second))
        {
            const size_t numInlierMatches = ( database->ExistsInlierMatches(estimatePair.first , estimatePair.second) ? database->ReadTwoViewGeometry(estimatePair.first , estimatePair.second).inlier_matches.size() : 0 );
            std::cout << ( boost::format("[Estimate] %1% - %2%: %3%") % estimatePair.first % estimatePair.second % numInlierMatches ).str() << std::endl;
            lock_guard<mutex> matchingPairsLock(matchingPairs_Mu);
            matchingPairs.at(estimatePair.second).erase(estimatePair.first);
            if (matchingPairs.at(estimatePair.second).empty()) // 已经匹配完了
            {
                matchingPairs.erase(estimatePair.second);
                imagesStatus.at(allImages.at(estimatePair.second)) = CImageStatusFlag::CExtracted;
                std::cout << ( boost::format("[Debug] Image %1% matched finished!") % estimatePair.second ).str() << std::endl;

                lock_guard<mutex> reconstructionTaskLock(reconstructionTasks_Mu);
                reconstructionTasks.insert(estimatePair.second);
            }
        }
#else
        if (estimateTasks.empty())
        {
            continue;
        }
        constexpr size_t invalidIndex = numeric_limits<size_t>::max();
        pair<size_t , size_t> estimatePair = { invalidIndex, invalidIndex };
        if (!estimateTasks.try_pop(estimatePair) || estimatePair.first == invalidIndex || estimatePair.second == invalidIndex)
        {
            continue;
        }

        Timer geometricVerificationTimer;
        geometricVerificationTimer.Start();

        CWorkflow::EstimateTwoViewGeometry(*database , estimatePair.first , estimatePair.second);
        const size_t geometricVerificationTime = geometricVerificationTimer.ElapsedMicroSeconds() / 1000;
        geometricVerificationTimes.at(estimatePair.second) += geometricVerificationTime;

        const size_t numInlierMatches = ( database->ExistsInlierMatches(estimatePair.first , estimatePair.second) ? database->ReadTwoViewGeometry(estimatePair.first , estimatePair.second).inlier_matches.size() : 0 );
#ifndef NOT_USE_GLOBAL_FEATURES
        //std::cout << (boost::format("[Estimate] %1% - %2%: %3%") % estimatePair.first % estimatePair.second % numInlierMatches).str() << std::endl;
#endif

        if (numMatchingPairs.at(estimatePair.second) > 0)
        {
            numMatchingPairs.at(estimatePair.second)--;
        }
        if (numMatchingPairs.at(estimatePair.second) == 0)
        {
            imagesStatus.at(allImages.at(estimatePair.second)) = CImageStatusFlag::CTwoViewGeometryEstimated;
            LOG(INFO) << ( boost::format("[Estimate] Image %1% estimated finished!") % estimatePair.second ).str();
            //std::cout << ( boost::format("[Estimate] Image %1% estimated finished!") % estimatePair.second ).str() << std::endl;
            reconstructionTasks.insert(estimatePair.second);
        }

#endif
    }
}

void CWorkflow::Reconstruct()
{
    size_t tryCount = 0;
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        if (!database || !reconstructor)
        {
            continue;
        }

#ifdef _DEBUG
        {
            lock_guard<mutex> lock(reconstructionTasks_Mu);
            if (reconstructionTasks.find(reconstructNextImageIndex) == reconstructionTasks.end())
            {
                continue;
            }
        }

        isBusy = true;
        cout << ( boost::format("[Reconstructor] %1%!") % reconstructNextImageIndex ).str() << endl;
        reconstructor->ReconstructImage(reconstructNextImageIndex);
        reconstructNextImageIndex++;
        isBusy = false;
#else
        if (reconstructionTasks.find(reconstructNextImageIndex) == reconstructionTasks.end())
        {
            tryCount++;
            if (tryCount >= 10 && reconstructionTasks.find(reconstructNextImageIndex + 1) != reconstructionTasks.end() && reconstructionTasks.find(reconstructNextImageIndex + 2) != reconstructionTasks.end())
            {
                tryCount = 0;
                reconstructNextImageIndex++;
            }
            continue;
        }
        tryCount = 0;
        isBusy = true;
        LOG(INFO) << (( boost::format("[Reconstructor] %1%!") % reconstructNextImageIndex ).str());
        reconstructor->ReconstructImage(reconstructNextImageIndex);
        reconstructNextImageIndex++;
        isBusy = false;
#endif
    }
}
// Deprecate：无法和Reconstructor在不同的线程运行，暂时停用
void CWorkflow::MeshingModel()
{
    if (true) {
        return;
    }
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        if (!database || !reconstructor)
        {
            continue;
        }


    }
}

void CWorkflow::WriteReg()
{
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(200));
#ifdef _DEBUG
        {
            scoped_lock lock(readTasks_Mu , SIFTExtractCPUTasks_Mu , SIFTExtractGPUTasks_Mu , SIFTMatchCPUTasks_Mu , SIFTMatchGPUTasks_Mu , estimateTasks_Mu);
            if (!isBusy && readTasks.empty() && SIFTExtractCPUTasks.empty() && SIFTExtractGPUTasks.empty() && SIFTMatchCPUTasks.empty() && SIFTMatchGPUTasks.empty() && estimateTasks.empty())
            {
                WriteReconstructStatus(true);
            }
            else
            {
                WriteReconstructStatus(false);
            }
        }
#else
        if (!isBusy && readTasks.empty() && SIFTExtractCPUTasks.empty() && SIFTExtractGPUTasks.empty() && SIFTMatchCPUTasks.empty() && SIFTMatchGPUTasks.empty() && estimateTasks.empty())
        {
            WriteReconstructStatus(true);
        }
        else
        {
            WriteReconstructStatus(false);
        }
#endif
    }
}
void CWorkflow::WriteReconstructStatus(bool status)
{
    HKEY hKey;
    // 打开注册表键
    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER , TEXT("Software\\RTPS") , 0 , KEY_WRITE , &hKey);
    const DWORD value = ( status ? 1 : 0 );
    if (openRes == ERROR_SUCCESS)
    {
        LONG setRes = RegSetValueEx(hKey , TEXT("IsCompleted") , 0 , REG_DWORD , reinterpret_cast<const BYTE*>( &value ) , sizeof(value));
        RegCloseKey(hKey);
    }
}
void CWorkflow::Retrieval()
{
    while (!isStop)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        if (!database)
        {
            continue;
        }
        if (retrievalTasks.empty())
        {
            continue;
        }
        size_t imageID = numeric_limits<size_t>::max();
        if (!retrievalTasks.try_pop(imageID) || imageID == numeric_limits<size_t>::max())
        {
            continue;
        }

        Timer retrievalTimer;
        retrievalTimer.Start();

        const vector<image_t> retrievalResult = globalFeatureRetriever->Retrieve(imageID);
        numMatchingPairs[imageID] = retrievalResult.size();

        const size_t retrievalTime = retrievalTimer.ElapsedMicroSeconds() / 1000;
        globalFeatureRetrievalTimes.at(imageID) = retrievalTime;

        string output = "retrieval result: " + database->ReadImage(imageID).Name() + " -> ";
        for (image_t i = 0; i < retrievalResult.size(); i++)
        {
            SIFTMatchGPUTasks.push({ retrievalResult[i], imageID });

            output += database->ReadImage(retrievalResult[i]).Name();
            if (i + 2 <= retrievalResult.size())
            {
                output += ", ";
            }
        }
        cout << output << endl;

        if (numMatchingPairs.find(imageID) != numMatchingPairs.end() && numMatchingPairs.at(imageID) > 0)
        {
            imagesStatus.at(allImages.at(imageID)) = CImageStatusFlag::CMatching;
        }
        else
        {
            imagesStatus.at(allImages.at(imageID)) = CImageStatusFlag::CMatched;
        }
    }
}