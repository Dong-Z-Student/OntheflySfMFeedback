#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <cstdint>
#include <functional>
#include <string>

struct RawPointRecord
{
	uint64_t raw_point_id;
	float x;
	float y;
	float z;
};

enum class SlotState : LONG
{
	Empty = 0,
	Producing = 1,
	Full = 2,
	Consuming = 3
};

struct SharedSlotBlock
{
	DWORD magic;
	DWORD version;
	LONG state;
	ULONGLONG sequence;
	RawPointRecord record;
};

class SharedMemoryChannel
{
public:
	explicit SharedMemoryChannel(const std::wstring& channelName);
	~SharedMemoryChannel();

	bool TryProduce(const std::function<RawPointRecord()>& producer);
	bool TryConsume(const std::function<void(const RawPointRecord&, ULONGLONG)>& consumer);
	bool WaitAndConsume(
		const std::function<void(const RawPointRecord&, ULONGLONG)>& consumer,
		DWORD timeoutMs = INFINITE);

private:
	static constexpr DWORD kMagic = 0x534C4F54; // 'SLOT'
	static constexpr DWORD kVersion = 1;

	void OpenOrCreate();
	void InitializeBlock();

	void Lock();
	void Unlock();

	DWORD WaitDataReady(DWORD timeoutMs);
	void SignalDataReady();

private:
	std::wstring channel_name_;

	HANDLE mapping_ = nullptr;
	HANDLE mutex_ = nullptr;
	HANDLE event_ = nullptr;
	SharedSlotBlock* block_ = nullptr;
};