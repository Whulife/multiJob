#include <multiJobThreadQueue.h>
#include <cstddef> // for std::nullptr
multiJobThreadQueue::multiJobThreadQueue(std::shared_ptr<multiJobQueue> jqueue)
:m_doneFlag(false)
{
   setJobQueue(jqueue);    
}
void multiJobThreadQueue::setJobQueue(std::shared_ptr<multiJobQueue> jqueue)
{
   std::lock_guard<std::mutex> lock(m_threadMutex);
   
   if (m_jobQueue == jqueue) return;
   
   pause();
   while(isRunning()&&!isPaused())
   {
      m_jobQueue->releaseBlock();
   }
   m_jobQueue = jqueue;
   resume();

   if(m_jobQueue) startThreadForQueue();
}

std::shared_ptr<multiJobQueue> multiJobThreadQueue::getJobQueue() 
{ 
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return m_jobQueue; 
}

const std::shared_ptr<multiJobQueue> multiJobThreadQueue::getJobQueue() const 
{ 
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return m_jobQueue; 
}

std::shared_ptr<multiJob> multiJobThreadQueue::currentJob() 
{ 
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return m_currentJob; 
}

void multiJobThreadQueue::cancelCurrentJob()
{
   std::lock_guard<std::mutex> lock(m_threadMutex);
   if(m_currentJob)
   {
      m_currentJob->cancel();
   }
}
bool multiJobThreadQueue::isValidQueue()const
{
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return (m_jobQueue!=nullptr);
}

void multiJobThreadQueue::run()
{
   bool firstTime = true;
   bool validQueue = true;
   std::shared_ptr<multiJob> job;
   do
   {
      interrupt();
      // osg::notify(osg::NOTICE)<<"In thread loop "<<this<<std::endl;
      validQueue = isValidQueue();
      job = nextJob();
      if (job&&!m_doneFlag)
      {
         {
            std::lock_guard<std::mutex> lock(m_threadMutex);
            m_currentJob = job;
         }
         
         // if the job is ready to execute
         if(job->isReady())
         {
            job->start();
         }
         {            
            std::lock_guard<std::mutex> lock(m_threadMutex);
            m_currentJob = 0;
         }
         job.reset();
      }
      
      if (firstTime)
      {
         multi::Thread::yieldCurrentThread();
         firstTime = false;
      }
   } while (!m_doneFlag&&validQueue);
   
   {            
      std::lock_guard<std::mutex> lock(m_threadMutex);
      m_currentJob = 0;
   }
   if(job&&m_doneFlag&&job->isReady())
   {
      job->cancel();
   }
   job = 0;
}

void multiJobThreadQueue::setDone(bool done)
{
   m_threadMutex.lock();
   if (m_doneFlag==done)
   {
      m_threadMutex.unlock();
      return;
   }
   m_doneFlag = done;
   m_threadMutex.unlock();
   if(done)
   {
      {
         std::lock_guard<std::mutex> lock(m_threadMutex);
         if (m_currentJob)
            m_currentJob->release();
      }
      
      if (m_jobQueue)
         m_jobQueue->releaseBlock();
   }
}

bool multiJobThreadQueue::isDone() const 
{ 
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return m_doneFlag; 
}

bool multiJobThreadQueue::isProcessingJob()const
{
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return (m_currentJob!=nullptr);
}

void multiJobThreadQueue::cancel()
{
   
   if( isRunning() )
   {
      {
         std::lock_guard<std::mutex> lock(m_threadMutex);
         m_doneFlag = true;
         if (m_currentJob)
         {
            m_currentJob->cancel();
         }
         
         if (m_jobQueue) 
         {
            m_jobQueue->releaseBlock();
         }
      }
      
      // then wait for the the thread to stop running.
      while(isRunning())
      {
#if 1
         {
            std::lock_guard<std::mutex> lock(m_threadMutex);
            
            if (m_jobQueue) 
            {
               m_jobQueue->releaseBlock();
            }
         }
#endif
         multi::Thread::yieldCurrentThread();
      }
   }
}

bool multiJobThreadQueue::isEmpty()const
{
   std::lock_guard<std::mutex> lock(m_threadMutex);
   return m_jobQueue->isEmpty();
}

multiJobThreadQueue::~multiJobThreadQueue()
{
   cancel();
}

void multiJobThreadQueue::startThreadForQueue()
{
   if(m_jobQueue)
   {
      if(!isRunning())
      {
         start();
      }
   }
}

bool multiJobThreadQueue::hasJobsToProcess()const
{
   bool result = false;
   {
      std::lock_guard<std::mutex> lock(m_threadMutex);
      result = (!m_jobQueue->isEmpty()||m_currentJob);
   }
   
   return result;
}

std::shared_ptr<multiJob> multiJobThreadQueue::nextJob()
{
   std::shared_ptr<multiJob> job;
   m_threadMutex.lock();
   std::shared_ptr<multiJobQueue> jobQueue = m_jobQueue;
   bool checkIfValid = !m_doneFlag&&jobQueue;
   m_threadMutex.unlock();
   if(checkIfValid)
   {
      job = jobQueue->nextJob(true);
   }
   return job;
}
