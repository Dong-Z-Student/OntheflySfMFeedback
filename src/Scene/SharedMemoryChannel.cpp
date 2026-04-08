#include "SharedMemoryChannel.h"

#include <stdexcept>

SharedMemoryChannel::SharedMemoryChannel(const std::wstring& channelName)
	: channel_name_(channelName)
{
	OpenOrCreate();
}

SharedMemoryChannel::~SharedMemoryChannel()
{
	if (block_) ::UnmapViewOfFile(block_);
	if (mapping_) ::CloseHandle(mapping_);
	if (mutex_) ::CloseHandle(mutex_);
	if (event_) ::CloseHandle(event_);
}

void SharedMemoryChannel::OpenOrCreate()
{
	std::wstring mappingName = L"Local\\" + channel_name_ + L"_Mapping";
	std::wstring mutexName = L"Local\\" + channel_name_ + L"_Mutex";
	std::wstring eventName = L"Local\\" + channel_name_ + L"_DataReady";

	mutex_ = ::CreateMutexW(nullptr, FALSE, mutexName.c_str());
	event_ = ::CreateEventW(nullptr, FALSE, FALSE, eventName.c_str());
	mapping_ = ::CreateFileMappingW(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		0,
		sizeof(SharedSlotBlock),
		mappingName.c_str());

	if (!mutex_ || !event_ || !mapping_)
	{
		throw std::runtime_error("Create shared object failed");
	}

	block_ = static_cast<SharedSlotBlock*>(
		::MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedSlotBlock)));

	if (!block_)
	{
		throw std::runtime_error("MapViewOfFile failed");
	}

	Lock();
	if (block_->magic != kMagic || block_->version != kVersion)
	{
		InitializeBlock();
	}
	Unlock();
}

void SharedMemoryChannel::InitializeBlock()
{
	::ZeroMemory(block_, sizeof(SharedSlotBlock));
	block_->magic = kMagic;
	block_->version = kVersion;
	block_->state = static_cast<LONG>(SlotState::Empty);
	block_->sequence = 0;
}

void SharedMemoryChannel::Lock()
{
	::WaitForSingleObject(mutex_, INFINITE);
}

void SharedMemoryChannel::Unlock()
{
	::ReleaseMutex(mutex_);
}

DWORD SharedMemoryChannel::WaitDataReady(DWORD timeoutMs)
{
	return ::WaitForSingleObject(event_, timeoutMs);
}

void SharedMemoryChannel::SignalDataReady()
{
	::SetEvent(event_);
}

bool SharedMemoryChannel::TryProduce(const std::function<RawPointRecord()>& producer)
{
	Lock();

	if (static_cast<SlotState>(block_->state) != SlotState::Empty)
	{
		Unlock();
		return false;
	}

	block_->state = static_cast<LONG>(SlotState::Producing);
	Unlock();

	RawPointRecord localRecord = producer();

	Lock();
	block_->record = localRecord;
	++block_->sequence;
	block_->state = static_cast<LONG>(SlotState::Full);
	Unlock();

	SignalDataReady();
	return true;
}

bool SharedMemoryChannel::TryConsume(
	const std::function<void(const RawPointRecord&, ULONGLONG)>& consumer)
{
	Lock();

	if (static_cast<SlotState>(block_->state) != SlotState::Full)
	{
		Unlock();
		return false;
	}

	RawPointRecord localRecord = block_->record;
	ULONGLONG localSequence = block_->sequence;

	block_->state = static_cast<LONG>(SlotState::Consuming);
	Unlock();

	consumer(localRecord, localSequence);

	Lock();
	block_->state = static_cast<LONG>(SlotState::Empty);
	Unlock();

	return true;
}

bool SharedMemoryChannel::WaitAndConsume(
	const std::function<void(const RawPointRecord&, ULONGLONG)>& consumer,
	DWORD timeoutMs)
{
	if (TryConsume(consumer))
	{
		return true;
	}

	DWORD ret = WaitDataReady(timeoutMs);
	if (ret != WAIT_OBJECT_0)
	{
		return false;
	}

	return TryConsume(consumer);
}