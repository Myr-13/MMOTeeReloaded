#include "thread_pool.h"

void CThreadWorker::Init()
{
	m_Running = true;
	m_InUse = false;
	m_Task = 0x0;
	m_Thread = std::thread([this]() {
		Run();
	});
}

void CThreadWorker::Run()
{
	while (m_Running)
	{
		if (!m_Task)
			std::this_thread::yield();
		else
		{
			m_InUse = true;
			m_Task();
			m_InUse = false;
		}
	}
}

void CThreadWorker::Stop()
{
	m_Running = false;
	m_Thread.join();
}

void CThreadWorker::SetTask(CThreadTask &Task)
{
	m_Task = std::move(Task);
	m_Thread.join();
}
