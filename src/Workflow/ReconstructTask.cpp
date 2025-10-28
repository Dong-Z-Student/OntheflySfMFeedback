#include "../Base/misc.h"
#include "../Base/timer.h"
#include "../Estimator/IncrementalMapper.h"
#include "glog/logging.h"
#include "ReconstructTask.h"
#include "WorkData.h"
#include <sstream>

void ReconstructTask::run()
{
	// bufferSize：缓冲区大小，反映了数据库更新的频率。缓冲区越小更新频率越高，重建速度也会变慢，缓冲区大则更新频率缓慢，处理速度会变快
	const int bufferSize = 50;
	const bool isCached = m_data->CheckCache(imageID);
	if (isCached) return;
	m_data->LoadCache(bufferSize);
	if (type == ReconstructType::AddModel) {
		modelID = m_data->ModelManagerAddModel();
		const bool success = Reconstruct(true);
		emit reconstructionInitial(modelID, success);
	}
	if (type == ReconstructType::AddImage) {
		const bool success = Reconstruct();
	}
}

bool ReconstructTask::Reconstruct(bool init)
{
	Timer reconstructionTimer;
	reconstructionTimer.Start();

	IncrementalMapper* mapper = m_data->getIncrementalMapper();

	std::shared_ptr<Reconstruction> model = m_data->getModelManager()->Get(modelID);
	mapper->BeginReconstruction(model);

	IncrementalMapper::Options init_mapper_options;

	// InitializeEmptyModel
	for (size_t numInitTrials = 0; numInitTrials < 2; numInitTrials++)
	{
		if (model->NumRegImages() > 0)
		{
			break;
		}
		// 只对空模型尝试初始化
		{
			QWriteLocker locker(&(m_data->getModelLock(modelID)));
			if (InitializeEmptyModel(init_mapper_options, *model, *mapper))
			{
				//lastRegModelID = modelID;
				//Callback(CHANGE_CURRENT_MODEL_CALLBACK);
				break;
			}

			init_mapper_options.init_min_num_inliers /= 2;
			if (InitializeEmptyModel(init_mapper_options, *model, *mapper))
			{
				//lastRegModelID = modelID;
				//Callback(CHANGE_CURRENT_MODEL_CALLBACK);
				break;
			}
		}
		init_mapper_options.init_min_tri_angle /= 2;
	}
	if (model->NumRegImages() == 0)
	{
		mapper->EndReconstruction(true);
		return false;
	}
	model->reconstructTime += (reconstructionTimer.ElapsedMicroSeconds() / 1000);

	// IncrementalMapping
	bool isContinue = true;
	while (isContinue)
	{
		reconstructionTimer.Restart();
		isContinue = false;
		size_t registerTime = 0;
		const std::vector<image_t> nextImageIDs = mapper->FindNextImages(init_mapper_options);
		registerTime += reconstructionTimer.ElapsedMicroSeconds() / 1000;
		model->reconstructTime += (reconstructionTimer.ElapsedMicroSeconds() / 1000);

		if (nextImageIDs.empty())
		{
			break;
		}

		std::vector<size_t> orderedNextImageIDs;
		if (!init)
		{
			std::unordered_set<size_t> nextImageSet(nextImageIDs.begin(), nextImageIDs.end());
			orderedNextImageIDs.reserve(nextImageIDs.size());
			for (size_t id : m_data->getImageIDsMask())
			{
				if (nextImageSet.count(id) > 0)
				{
					orderedNextImageIDs.push_back(id);
				}
			}
		}
		else
		{
			orderedNextImageIDs.resize(nextImageIDs.size());
			std::transform(nextImageIDs.begin(), nextImageIDs.end(), orderedNextImageIDs.begin(),
				[](image_t id) { return static_cast<size_t>(id); });
		}

		size_t numRegNextImageTrials = 0;
		for (size_t nextImageID : orderedNextImageIDs)
		{
			numRegNextImageTrials++;
			if (numRegNextImageTrials > 10)
			{
				break;
			}

			reconstructionTimer.Restart();
			const std::string nextImageName = m_data->DatabaseReadImage(nextImageID).Name();
			{
				QWriteLocker locker(&(m_data->getModelLock(modelID)));
				if (!mapper->RegisterNextImage(init_mapper_options, nextImageID))
				{
					model->reconstructTime += (reconstructionTimer.ElapsedMicroSeconds() / 1000);
					LOG(INFO) << "[model " << (modelID + 1) << "] image " << nextImageName << " registered failed!";
					continue;
				}
			}
			model->reconstructTime += (reconstructionTimer.ElapsedMicroSeconds() / 1000);
			registerTime += reconstructionTimer.ElapsedMicroSeconds() / 1000;
			model->registerTimes.push_back(registerTime);
			LOG(INFO) << "[model " << (modelID + 1) << "] image " << nextImageName << " registered successfully!";


			reconstructionTimer.Restart();
			TriangulateImage(init_mapper_options, nextImageID, *mapper);
			model->reconstructTime += (reconstructionTimer.ElapsedMicroSeconds() / 1000);
			model->triangulateTimes.push_back(reconstructionTimer.ElapsedMicroSeconds() / 1000);

			reconstructionTimer.Restart();
			LocalBundleAdjustment(init_mapper_options, nextImageID, *mapper);
			ExtractColors(init_mapper_options, nextImageID, *model);

			emit registedSuccess(nextImageID);
			model->reconstructTime += (reconstructionTimer.ElapsedMicroSeconds() / 1000);

#ifdef GaussianSplatting_Output
			if (modelID != 0)
			{
				continue;
			}
			const size_t numRegImages = modelManager->Get(modelID)->NumRegImages();
			if (numRegImages % 1 == 0 && numRegImages != 0 && numRegImages > 1)
			{
				const std::string basePath = StringReplace(EnsureTrailingSlash(GetParentDir(Database::exportPath)), "\\", "/");

				const std::string modelExportDir = basePath + "models/" + to_string(numRegImages);
				CreateDirIfNotExists(modelExportDir + "/bin", true);
				CreateDirIfNotExists(modelExportDir + "/text", true);
				modelManager->Get(modelID)->WriteBinary(modelExportDir + "/bin");
				modelManager->Get(modelID)->WriteText(modelExportDir + "/text");
				modelManager->Get(modelID)->OutputDebugResult(modelExportDir);

				//const size_t imageNum = database->NumImages();
				//const size_t imageOutNum = (imageNum / 20) * 20;
				const size_t imageOutNum = numRegImages;
				const vector<image_t> RegImagesIds = modelManager->Get(modelID)->RegImageIds();

				vector<pair<image_t, image_t>> imagePairs(0);
				vector<int> numInliers(0);
				database->ReadTwoViewGeometryNumInliers(&imagePairs, &numInliers);
				for (int i = 0; i < imagePairs.size(); i++)
				{
					const image_pair_t imagePairID = Database::ImagePairToPairId(imagePairs[i].first, imagePairs[i].second);
					const size_t numInlierMatches = numInliers[i];
					matchMatrix[imagePairID] = numInlierMatches;
				}

				std::ofstream MatchMatrixImageName(modelExportDir + "/match_matrix_image_name.txt");
				for (size_t i = 1; i <= imageOutNum; i++)
				{
					if (i < imageOutNum)
					{
						const image_t imageId = RegImagesIds[i - 1];
						MatchMatrixImageName << RemoveExtension(database->ReadImage(imageId).Name()) << ",";
					}
					if (i == imageOutNum)
					{
						const image_t imageId = RegImagesIds[i - 1];
						MatchMatrixImageName << RemoveExtension(database->ReadImage(imageId).Name()) << endl;
					}

				}
				MatchMatrixImageName.close();

				std::ofstream MatchMatrixValue(modelExportDir + "/match_matrix_value.txt");
				for (size_t i = 1; i <= imageOutNum; i++)
				{
					for (size_t j = 1; j <= imageOutNum; j++)
					{
						const image_t imageID1 = RegImagesIds[i - 1];
						const image_t imageID2 = RegImagesIds[j - 1];
						const size_t value = GetMatrixValue(imageID1, imageID2);
						if (j < imageOutNum)
						{
							if (j != i)
							{
								MatchMatrixValue << value << ",";
							}
							if (j == i)
							{
								MatchMatrixValue << database->NumKeypointsForImage(imageID1) << ",";
							}
						}
						if (j == imageOutNum)
						{
							if (j != i)
							{
								MatchMatrixValue << value;
							}
							if (j == i)
							{
								MatchMatrixValue << database->NumKeypointsForImage(imageID1);
							}
						}
					}
					MatchMatrixValue << endl;
				}

				cout << (boost::format("Export %1% models completed!") % numRegImages).str() << endl;
			}
#endif

			isContinue = true;
		}
	}
	mapper->EndReconstruction(false);
	return true;
}

bool ReconstructTask::InitializeEmptyModel(const IncrementalMapper::Options& options, Reconstruction& model, IncrementalMapper& mapper)
{
	LOG(INFO) << "Finding good initial image pair...";

	image_t initImageID1 = std::numeric_limits<image_t>::max();
	image_t initImageID2 = std::numeric_limits<image_t>::max();
	const bool isFindInitSuccess = mapper.FindInitialImagePair(options, &initImageID1, &initImageID2);
	if (!isFindInitSuccess)
	{
		LOG(INFO) << "No good initial image pair found!";
		return false;
	}
	//if (!database->ExistsImage(initImageID1) || !database->ExistsImage(initImageID2))
	//{
	//	cout << (boost::format("Initial image pair %1% and %2% do not exist!") % initImageID1 % initImageID2).str() << endl;
	//	return false;
	//}

	LOG(INFO) << "Initializing with image pair " << initImageID1 << " and " << initImageID2 << "...";
	const bool isRegInitSuccess = mapper.RegisterInitialImagePair(options, initImageID1, initImageID2);
	if (!isRegInitSuccess)
	{
		LOG(INFO) << "Initialization failed!";
		return false;
	}
	LOG(INFO) << "Initialization successful!";

	GlobalBundleAdjustment(options, mapper);
	FilterPoints(options, mapper);
	FilterImages(options, mapper);

	if (model.NumRegImages() == 0 || model.NumPoints3D() == 0)
	{
		model.DeRegisterImage(initImageID1);
		model.DeRegisterImage(initImageID2);
		LOG(INFO) << "After bundle adjustment, the registered images or 3D points is empty!";
		return false;
	}
	ExtractColors(options, initImageID1, model);
	//Callback(INITIAL_IMAGE_PAIR_REG_CALLBACK);
	emit registedInitialImagePair(initImageID1, initImageID2);
	return true;
}

void ReconstructTask::LocalBundleAdjustment(const IncrementalMapper::Options& options, size_t imageID, IncrementalMapper& mapper)
{
	BundleAdjustmentOptions custom_ba_options;
	IncrementalMapper::Options localBundleAdjustOptions = options;
	IncrementalTriangulator::Options triOptions;

#ifdef USE_HIERARCHICAL_WEIGHT_BA
	const size_t numLocalBAIters = 2;
#else
	const size_t numLocalBAIters = 2;
#endif

	std::vector<double> localBAErrors;
	std::vector<size_t> localBATimes;

	for (size_t i = 0; i < numLocalBAIters; i++)
	{
		Timer localBATimer;
		localBATimer.Start();

		double finalCost = 0;
		const IncrementalMapper::LocalBundleAdjustmentReport report = mapper.AdjustLocalBundle(localBundleAdjustOptions, custom_ba_options, triOptions, imageID, mapper.GetModifiedPoints3D(), finalCost);
		double changed = 0;
		if (report.num_adjusted_observations > 0)
		{
			changed = (report.num_merged_observations + report.num_completed_observations + report.num_filtered_observations) * 1.0 / report.num_adjusted_observations;
		}

#ifdef _DEBUG
		cout << (boost::format("[Reconstructor] Merged observations: %1%") % report.num_merged_observations).str() << endl;
		cout << (boost::format("[Reconstructor] Completed observations: %1%") % report.num_completed_observations).str() << endl;
		cout << (boost::format("[Reconstructor] Filtered observations: %1%") % report.num_filtered_observations).str() << endl;
		cout << (boost::format("[Reconstructor] Changed observations: %1%") % changed).str() << endl;
#endif

		const size_t localBATime = localBATimer.ElapsedMicroSeconds() / 1000;
		if (changed < 0.001f)
		{
			localBATimes.push_back(localBATime);
			localBAErrors.push_back(finalCost);
			break;
		}
		custom_ba_options.loss_function_type = BundleAdjustmentOptions::LossFunctionType::TRIVIAL;

		localBATimes.push_back(localBATime);
		localBAErrors.push_back(finalCost);
	}
	mapper.ClearModifiedPoints3D();

	mapper.GetReconstruction()->localBAErrors.push_back(localBAErrors);
	mapper.GetReconstruction()->localBATimes.push_back(localBATimes);
}

void ReconstructTask::GlobalBundleAdjustment(const IncrementalMapper::Options& options, IncrementalMapper& mapper)
{
	Timer globalBATimer;
	globalBATimer.Start();

	BundleAdjustmentOptions custom_ba_options;
	IncrementalMapper::Options globalBundleAdjustOptions = options;
	if (mapper.GetReconstruction()->NumRegImages() < 10)
	{
		custom_ba_options.solver_options.function_tolerance /= 10;
		custom_ba_options.solver_options.gradient_tolerance /= 10;
		custom_ba_options.solver_options.parameter_tolerance /= 10;
		custom_ba_options.solver_options.max_num_iterations *= 2;
		custom_ba_options.solver_options.max_linear_solver_iterations = 200;
	}
	custom_ba_options.solver_options.function_tolerance = 0;
	custom_ba_options.solver_options.gradient_tolerance = 1.0;
	custom_ba_options.solver_options.parameter_tolerance = 0.0;
	custom_ba_options.solver_options.max_num_iterations = 50;
	custom_ba_options.solver_options.max_linear_solver_iterations = 100;
	custom_ba_options.solver_options.minimizer_progress_to_stdout = true;

	custom_ba_options.solver_options.minimizer_progress_to_stdout = true;
	custom_ba_options.solver_options.logging_type = ceres::LoggingType::SILENT;

	double finalCost = 0;
	mapper.AdjustGlobalBundle(globalBundleAdjustOptions, custom_ba_options, finalCost);
	LOG(INFO) << "[Global BA] final cost: " << finalCost << "px";

	mapper.GetReconstruction()->finalGlobalBATime = globalBATimer.ElapsedMicroSeconds() / 1000;
	mapper.GetReconstruction()->finalGloablBAError = finalCost;
}

size_t ReconstructTask::TriangulateImage(const IncrementalMapper::Options& options, size_t imageID, IncrementalMapper& mapper)
{
	IncrementalTriangulator::Options triOptions;
	const size_t numTris = mapper.TriangulateImage(triOptions, imageID);
	return numTris;
}

size_t ReconstructTask::FilterPoints(const IncrementalMapper::Options& options, IncrementalMapper& mapper)
{
	const size_t numFilterPoints = mapper.FilterPoints(options);
	return numFilterPoints;
}

size_t ReconstructTask::FilterImages(const IncrementalMapper::Options& options, IncrementalMapper& mapper)
{
	const size_t numFilterImages = mapper.FilterImages(options);
	return numFilterImages;
}

void ReconstructTask::ExtractColors(const IncrementalMapper::Options& options, size_t imageID, Reconstruction& model)
{
	const std::string imageDir = EnsureTrailingSlash(StringReplace(Database::imageDir, "\\", "/"));
	const std::string imagePath = imageDir + m_data->DatabaseReadImage(imageID).Name();
	if (model.ExtractColorsForImage(imageID, imageDir))
	{
		return;
	}
	LOG(INFO) << "[Reconstructor] Could not read image " << imagePath;
}

void ReconstructTask::onRegistedInitialImagePair(quint32 imageID1, quint32 imageID2) {

}

void ReconstructTask::onRegistedSuccess(quint32 p_imageID) {

}
