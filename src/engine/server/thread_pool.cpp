#include "thread_pool.h"

CThreadPool::~CThreadPool()
{
	for (CThreadWorker *pWorker : m_vpWorkers)
	{
		pWorker->Stop();
		delete pWorker;
	}
}

void CThreadPool::Init(int NumWorkers)
{
	for (int i = 0; i < NumWorkers; i++)
	{
		CThreadWorker *pWorker = new CThreadWorker();
		pWorker->Init();
		m_vpWorkers.push_back(pWorker);
	}
}

void CThreadPool::AddTask(const CThreadTask& Func)
{
	m_Tasks.push(Func);

	ExecuteNextTask();
}

void CThreadPool::GetTask(CThreadTask &Task)
{
	Task = std::move(m_Tasks.front());
	m_Tasks.pop();
}

void CThreadPool::ExecuteNextTask()
{
	if (m_Tasks.empty())
		return;

	for (CThreadWorker *pWorker : m_vpWorkers)
	{
		if (!pWorker->InUse())
		{
			CThreadTask Task;
			GetTask(Task);

			pWorker->SetTask(Task);
			pWorker->Run();
		}
	}
}
