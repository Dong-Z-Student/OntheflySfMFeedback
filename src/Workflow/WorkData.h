#ifndef WORKDATA_H
#define WORKDATA_H

#include "../Feature/Bitmap.h"
#include "../Feature/FeatureExtractor.h"
#include "../Feature/FeatureMatcher.h"
#include "../Feature/ImageReader.h"
#include "../Scene/Database.h"
#include "tbb/concurrent_map.h"
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QReadWriteLock>

enum CImageStatusFlag : char {
	CUnread = 0,
	CRead = 1,
	CExtracting = 2,
	CExtracted = 3,
	CMatching = 4,
	CMatched = 5,
	CTwoViewGeometryEstimating = 6,
	CTwoViewGeometryEstimated = 7
};

namespace {
	void ScaleKeypoints(const Bitmap& bitmap, const Camera& camera, FeatureKeypoints* keypoints) {
		if (static_cast<size_t>(bitmap.Width()) != camera.Width() ||
			static_cast<size_t>(bitmap.Height()) != camera.Height()) {

			const float scale_x = static_cast<float>(camera.Width()) / bitmap.Width();
			const float scale_y = static_cast<float>(camera.Height()) / bitmap.Height();

			for (auto& keypoint : *keypoints) {
				keypoint.Rescale(scale_x, scale_y);
			}
		}
	}
}

class WorkData : public QObject {
	Q_OBJECT

public:
	explicit WorkData(QObject* parent = nullptr)
		: QObject(parent),
		database(nullptr),
		globalFeatureExtractor(nullptr),
		globalFeatureRetriever(nullptr) {}

	void Initial(Database* p_database) {
		database = p_database;
	}

	//// 对外提供公共资源操作方法
	// set/get
	Database* getDatabase() const { return database; }
	CGlobalFeatureExtractor* getGlobalFeatureExtractor() const { return globalFeatureExtractor; }
	CGlobalFeatureRetriever* getGlobalFeatureRetriever() const { return globalFeatureRetriever; }

	void setGlobalFeatureFunc(CGlobalFeatureExtractor* p_globalFeatureExtractor,
		CGlobalFeatureRetriever* p_globalFeatureRetriever) {
		globalFeatureExtractor = p_globalFeatureExtractor;
		globalFeatureRetriever = p_globalFeatureRetriever;
	}

	//// === Database 相关操作 ===
	camera_t DatabaseWriteCamera(Camera camera) {
		QWriteLocker locker(&m_database_lock);
		return database->WriteCamera(camera);
	}

	image_t DatabaseWriteImage(Image image) {
		QWriteLocker locker(&m_database_lock);
		return database->WriteImage(image);
	}

	void DatabaseWriteKeypoints(image_t imageid, FeatureKeypoints& p_keypoints) {
		QWriteLocker locker(&m_database_lock);
		database->WriteKeypoints(imageid, p_keypoints);
	}

	void DatabaseWriteDescriptors(image_t imageid, FeatureDescriptors& p_descriptors) {
		QWriteLocker locker(&m_database_lock);
		database->WriteDescriptors(imageid, p_descriptors);
	}

	Camera DatabaseReadCamera(camera_t cameraid) {
		QReadLocker locker(&m_database_lock);
		return database->ReadCamera(cameraid);
	}

	Image DatabaseReadImage(image_t imageid) {
		QReadLocker locker(&m_database_lock);
		return database->ReadImage(imageid);
	}

	void DatabaseWriteGlobalFeature(image_t imageID, GlobalFeature& globalFeature) {
		QWriteLocker locker(&m_database_lock);
		database->WriteGlobalFeature(imageID, globalFeature);
	}

	FeatureDescriptors DatabaseReadDescriptors(image_t imageID) {
		QReadLocker locker(&m_database_lock);
		return database->ReadDescriptors(imageID);
	}

	void DatabaseWriteMatches(image_t id1, image_t id2, FeatureMatches& matches) {
		QWriteLocker locker(&m_database_lock);
		database->WriteMatches(id1, id2, matches);
	}

	bool DatabaseExistsMatches(image_t id1, image_t id2) {
		QReadLocker locker(&m_database_lock);
		return database->ExistsMatches(id1, id2);
	}

	FeatureMatches DatabaseReadMatches(image_t id1, image_t id2) {
		QReadLocker locker(&m_database_lock);
		return database->ReadMatches(id1, id2);
	}

	FeatureKeypoints DatabaseReadKeypoints(image_t imageid) {
		QReadLocker locker(&m_database_lock);
		return database->ReadKeypoints(imageid);
	}

	void DatabaseWriteTwoViewGeometry(image_t id1, image_t id2, const TwoViewGeometry& twoviewgeo) {
		QWriteLocker locker(&m_database_lock);
		database->WriteTwoViewGeometry(id1, id2, twoviewgeo);
	}

	bool DatabaseExistsInlierMatches(image_t id1, image_t id2) {
		QReadLocker locker(&m_database_lock);
		return database->ExistsInlierMatches(id1, id2);
	}

	TwoViewGeometry DatabaseReadTwoViewGeometry(image_t id1, image_t id2) {
		return database->ReadTwoViewGeometry(id1, id2);
	}

	//// === Image 相关操作 ===
	image_t getImageID(const std::string imagePath) {
		QReadLocker locker(&m_imageidpath_lock);
		return allImagePaths.at(imagePath);
	}

	std::string getImagePath(const image_t imageID) {
		QReadLocker locker(&m_imageidpath_lock);
		return allImages.at(imageID);
	}

	void addImageMap(image_t imageid, std::string imagepath) {
		QWriteLocker locker(&m_imageidpath_lock);
		allImages[imageid] = imagepath;
		allImagePaths[imagepath] = imageid;
	}

	void setImageStatus(std::string imagepath, CImageStatusFlag status) {
		QMutexLocker locker(&m_imagestatus_lock);
		imagesStatus[imagepath] = status;
	}

	//// === Bitmap 相关操作 ===
	void WriteBitmap(Bitmap& p_bitmap, image_t imageid) {
		QWriteLocker locker(&m_bitmap_lock);
		allBitmaps[imageid] = p_bitmap;
	}

	Bitmap ReadBitmap(image_t imageid) {
		QReadLocker locker(&m_bitmap_lock);
		return allBitmaps.at(imageid);
	}

	void ReleaseBitmap(image_t imageid) {
		QWriteLocker locker(&m_bitmap_lock);
		allBitmaps.erase(imageid);
	}

public:
	//// 并发计时器
	tbb::concurrent_map<std::size_t, std::atomic_size_t> SIFTExtractionTimes;
	tbb::concurrent_map<std::size_t, std::atomic_size_t> globalFeatureExtractionTimes;
	tbb::concurrent_map<std::size_t, std::atomic_size_t> globalFeatureRetrievalTimes;
	tbb::concurrent_map<std::size_t, std::atomic_size_t> SIFTMatchingTimes;
	tbb::concurrent_map<std::size_t, std::atomic_size_t> geometricVerificationTimes;

	tbb::concurrent_unordered_map<size_t, std::atomic_size_t> numMatchingPairs;

private:
	//// 工作变量
	Database* database;
	CGlobalFeatureExtractor* globalFeatureExtractor;
	CGlobalFeatureRetriever* globalFeatureRetriever;

	std::unordered_map<std::size_t, std::string> allImages;
	std::unordered_map<std::string, std::size_t> allImagePaths;
	std::unordered_map<std::string, CImageStatusFlag> imagesStatus;
	std::unordered_map<std::size_t, Bitmap> allBitmaps;

	//// 互斥锁
	mutable QReadWriteLock m_database_lock;
	mutable QReadWriteLock m_imageidpath_lock;
	mutable QReadWriteLock m_bitmap_lock;
	QMutex m_imagestatus_lock;
};

#endif // WORKDATA_H
