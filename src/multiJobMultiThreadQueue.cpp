#include <multiJobMultiThreadQueue.h>

multiJobMultiThreadQueue::multiJobMultiThreadQueue(std::shared_ptr<multiJobQueue> q, 
                                                   unsigned int nThreads)
:m_jobQueue(q?q:std::make_shared<multiJobQueue>())
{
   setNumberOfThreads(nThreads);
}

multiJobMultiThreadQueue::~multiJobMultiThreadQueue()
{
   cancel();
   waitForCompletion();
   m_threadQueueList.clear();
}

std::shared_ptr<multiJobQueue> multiJobMultiThreadQueue::getJobQueue()
{
   std::lock_guard<std::mutex> lock(m_mutex);
   return m_jobQueue;
}
const std::shared_ptr<multiJobQueue> multiJobMultiThreadQueue::getJobQueue()const
{
   std::lock_guard<std::mutex> lock(m_mutex);
   return m_jobQueue;
}
void multiJobMultiThreadQueue::setJobQueue(std::shared_ptr<multiJobQueue> q)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   unsigned int idx = 0;
   m_jobQueue = q;
   for(idx = 0; idx < m_threadQueueList.size(); ++idx)
   {
      m_threadQueueList[idx]->setJobQueue(m_jobQueue);
   }
}
void multiJobMultiThreadQueue::setNumberOfThreads(unsigned int nThreads)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   size_t idx = 0;
   size_t queueSize = m_threadQueueList.size();

   
   if(nThreads > queueSize)
   {
      for(idx = queueSize; idx < nThreads;++idx)
      {
         std::shared_ptr<multiJobThreadQueue> threadQueue = std::make_shared<multiJobThreadQueue>();
         threadQueue->setJobQueue(m_jobQueue);
         m_threadQueueList.push_back(threadQueue);
      }
   }
   else if(nThreads < queueSize)
   {
      ThreadQueueList::iterator iter = m_threadQueueList.begin()+nThreads;
      while(iter != m_threadQueueList.end())
      {
         (*iter)->cancel();
         iter = m_threadQueueList.erase(iter);
      }
   }
}

unsigned int multiJobMultiThreadQueue::getNumberOfThreads() const
{
   std::lock_guard<std::mutex> lock(m_mutex);
   return static_cast<unsigned int>( m_threadQueueList.size() );
}

unsigned int multiJobMultiThreadQueue::numberOfBusyThreads()const
{
   unsigned int result = 0;
   std::lock_guard<std::mutex> lock(m_mutex);
   unsigned int idx = 0;
   size_t queueSize = m_threadQueueList.size();
   for(idx = 0; idx < queueSize;++idx)
   {
      if(m_threadQueueList[idx]->isProcessingJob()) ++result;
   }
   return result;
}

bool multiJobMultiThreadQueue::areAllThreadsBusy()const
{
   std::lock_guard<std::mutex> lock(m_mutex);
   unsigned int idx = 0;
   size_t queueSize = m_threadQueueList.size();
   for(idx = 0; idx < queueSize;++idx)
   {
      if(!m_threadQueueList[idx]->isProcessingJob()) return false;
   }
   
   return true;
}

bool multiJobMultiThreadQueue::hasJobsToProcess()const
{
   bool result = false;
   {
      std::lock_guard<std::mutex> lock(m_mutex);
      size_t queueSize = m_threadQueueList.size();
      unsigned int idx = 0;
      for(idx = 0; ((idx<queueSize)&&!result);++idx)
      {
         result = m_threadQueueList[idx]->hasJobsToProcess();
      }
   }
   
   return result;
}

void multiJobMultiThreadQueue::cancel()
{
   for(auto thread:m_threadQueueList)
   {
      thread->cancel();
   }
}

void multiJobMultiThreadQueue::waitForCompletion()
{
   for(auto thread:m_threadQueueList)
   {
      thread->waitForCompletion();
   }
}

