#ifndef ENGINE_SERVER_THREAD_POOL_H
#define ENGINE_SERVER_THREAD_POOL_H

#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <chrono>
#include <queue>
#include <thread>

typedef std::function<void()> CThreadTask;

class CThreadWorker
{
	std::thread m_Thread;
	CThreadTask m_Task;
	bool m_InUse;
	bool m_Running;

public:
	void Init();
	void Run();
	void Stop();
	void SetTask(CThreadTask &Task);

	bool InUse() { return m_InUse; }
	void SetUse(bool Use) { m_InUse = Use; }
};

class CThreadPool
{
	std::vector<CThreadWorker *> m_vpWorkers;
	std::queue<CThreadTask> m_Tasks;

public:
	~CThreadPool();

	void Init(int NumWorkers);
	void AddTask(const CThreadTask& Func);
	void GetTask(CThreadTask &Task);
	void ExecuteNextTask();
};

#endif // ENGINE_SERVER_THREAD_POOL_H
